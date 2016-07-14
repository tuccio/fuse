#include "renderer_application.hpp"

#include <fuse/core.hpp>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <fuse/gpu_upload_manager.hpp>
#include <fuse/gpu_graphics_command_list.hpp>

#include <fuse/render_resource.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/image_manager.hpp>
#include <fuse/material_manager.hpp>
#include <fuse/mesh_manager.hpp>
#include <fuse/gpu_mesh_manager.hpp>
#include <fuse/texture_manager.hpp>
#include <fuse/bitmap_font_manager.hpp>

#include <fuse/render_resource_manager.hpp>

#include <fuse/assimp_loader.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/text_renderer.hpp>
#include <fuse/mipmap_generator.hpp>

#include <fuse/camera.hpp>
#include <fuse/fps_camera_controller.hpp>
#include <fuse/descriptor_heap.hpp>
#include <fuse/rocket_interface.hpp>

#include <Rocket/Controls.h>

#include "alpha_composer.hpp"
#include "cbuffer_structs.hpp"
#include "debug_renderer.hpp"
#include "visual_debugger.hpp"
#include "deferred_renderer.hpp"
#include "scene.hpp"
#include "shadow_mapper.hpp"
#include "tonemapper.hpp"
#include "wx_editor_gui.hpp"
#include "render_variables.hpp"
#include "blur.hpp"
#include "sdsm.hpp"
#include "realtime_renderer.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define FAIL_IF(Condition) { if (Condition) return false; }

#define MAX_DSV 8
#define MAX_RTV 1024
#define MAX_SRV 1024

#define SKYDOME_WIDTH  2048
#define SKYDOME_HEIGHT 768

#define NUM_BUFFERS      (2u)
#define UPLOAD_HEAP_SIZE (16 << 20)

#define FUSE_UI_CONFIGURATION "ui/editorui.conf"

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_cbPerFrame;

std::array<descriptor_token_t, NUM_BUFFERS> g_gbufferSRVTable;

std::array<render_resource, NUM_BUFFERS> g_sdsmConstantBuffer;

std::unique_ptr<mipmap_generator> g_mipmapGenerator;
std::unique_ptr<assimp_loader>    g_sceneLoader;
								       
resource_factory                     g_resourceFactory;
std::unique_ptr<image_manager>       g_imageManager;
std::unique_ptr<mesh_manager>        g_meshManager;
std::unique_ptr<material_manager>    g_materialManager;
std::unique_ptr<gpu_mesh_manager>    g_gpuMeshManager;
std::unique_ptr<texture_manager>     g_textureManager;
std::unique_ptr<bitmap_font_manager> g_bitmapFontManager;

std::array<gpu_graphics_command_list, NUM_BUFFERS> g_guiCommandList;

render_resource_manager g_renderResourceManager;

fps_camera_controller g_cameraController;


bitmap_font_ptr    g_font;

scene              g_scene;

realtime_renderer g_realtimeRenderer;
text_renderer     g_textRenderer;
tonemapper        g_tonemapper;
sdsm              g_sdsm;

render_configuration g_renderConfiguration;

visual_debugger g_visualDebugger;

alpha_composer g_alphaComposer;

D3D12_VIEWPORT g_fullscreenViewport;
D3D12_RECT     g_fullscreenScissorRect;

bool g_initialized;

#ifdef FUSE_USE_EDITOR_GUI
wx_editor_gui g_wxEditorGUI;
#endif

#ifdef FUSE_LIBROCKET
rocket_interface g_rocketInterface;
#endif

bool renderer_application::on_render_context_created(gpu_render_context & renderContext)
{

	maximize();

	ID3D12Device              * device       = renderContext.get_device();
	gpu_command_queue         & commandQueue = renderContext.get_command_queue();
	gpu_graphics_command_list & commandList  = renderContext.get_command_list();

	// The command list will be used for uploading assets and buffers on the gpu
	commandList.reset_command_list(nullptr);

	g_mipmapGenerator = std::make_unique<mipmap_generator>(device);

	/* Resource managers */

	g_imageManager      = std::make_unique<image_manager>();
	g_materialManager   = std::make_unique<material_manager>();
	g_meshManager       = std::make_unique<mesh_manager>();
	g_bitmapFontManager = std::make_unique<bitmap_font_manager>();
	g_gpuMeshManager    = std::make_unique<gpu_mesh_manager>();
	g_textureManager    = std::make_unique<texture_manager>();

	g_resourceFactory.register_manager(g_imageManager.get());
	g_resourceFactory.register_manager(g_materialManager.get());
	g_resourceFactory.register_manager(g_bitmapFontManager.get());
	g_resourceFactory.register_manager(g_meshManager.get());
	g_resourceFactory.register_manager(g_gpuMeshManager.get());
	g_resourceFactory.register_manager(g_textureManager.get());

	/* Load scene */

	//g_sceneLoader = std::make_unique<assimp_loader>("scene/default.fbx");
	
	g_sceneLoader = std::make_unique<assimp_loader>(FUSE_LITERAL("scene/cornellbox_solid.fbx"));
	//g_sceneLoader = std::make_unique<assimp_loader>("scene/sponza.fbx");

	FAIL_IF(
		!g_scene.import_static_objects(g_sceneLoader.get()) ||
		!g_scene.import_cameras(g_sceneLoader.get()) ||
		//!g_scene.import_lights(g_sceneLoader.get()) ||
		!g_scene.get_skydome()->init(device, SKYDOME_WIDTH, SKYDOME_HEIGHT, NUM_BUFFERS)
	)

	g_scene.get_skydome()->set_zenith(1.02539599f);
	g_scene.get_skydome()->set_azimuth(-1.70169306f);

	g_scene.fit_octree(1.1f);
	
	/*auto light = *g_scene.get_lights().first;

	light->direction = XMFLOAT3(-0.281581f, 0.522937f, -0.804518f);
	light->intensity *= .2f;*/
	//g_scene.get_active_camera()->set_zfar(10000.f);

	//g_cameraController = std::make_unique<fps_camera_controller>(g_scene.get_active_camera());

	g_cameraController.set_camera(g_scene.get_active_camera());
	g_cameraController.set_speed(float3(200, 200, 200));
	
	add_keyboard_listener(&g_cameraController);
	add_mouse_listener(&g_cameraController);

	g_scene.get_active_camera()->set_orientation(quaternion(1, 0, 0, 0));
	//g_scene.get_active_camera()->set_position(float3(15, 135, -115));

	/* Create gbuffer desc tables */

	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		g_gbufferSRVTable[i] = cbv_uav_srv_descriptor_heap::get_singleton_pointer()->allocate(4);
	}

	/* Create constant buffers */

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		FAIL_IF (!gpu_global_resource_state::get_singleton_pointer()->
			create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cb_per_frame)),
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				nullptr,
				IID_PPV_ARGS(&g_cbPerFrame[i])
			)
		)

		FAIL_IF (!g_sdsmConstantBuffer[i].
			create(
				device,
				&CD3DX12_RESOURCE_DESC::Buffer(
					sdsm::get_constant_buffer_size(),
					D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				nullptr
			)
		)

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		uavDesc.Buffer.NumElements         = 1;
		uavDesc.Buffer.StructureByteStride = sdsm::get_constant_buffer_size();
		uavDesc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;

		g_sdsmConstantBuffer[i].create_unordered_access_view(device, &uavDesc);

	}

	g_font = resource_factory::get_singleton_pointer()->create<bitmap_font>(FUSE_LITERAL("bitmap_font"), FUSE_LITERAL("fonts/consolas_regular_12"));

	/* Initialize renderer */

	deferred_renderer_configuration rendererCFG;

	rendererCFG.gbufferFormat[0]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[1]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[2]       = DXGI_FORMAT_R8G8B8A8_UNORM;
	rendererCFG.gbufferFormat[3]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.dsvFormat              = DXGI_FORMAT_D24_UNORM_S8_UINT;
	rendererCFG.shadingFormat          = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.shadowMappingAlgorithm = g_renderConfiguration.get_shadow_mapping_algorithm();

	FAIL_IF(!create_command_lists(g_guiCommandList.begin(), g_guiCommandList.end(), device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, nullptr));

	for (auto & commandList : g_guiCommandList) FAIL_IF(FUSE_HR_FAILED(commandList->Close()));

#ifdef FUSE_USE_LIBROCKET

	FAIL_IF(
		!g_rocketInterface.init(device, rocketCFG)
	)

	Rocket::Core::SetRenderInterface(&g_rocketInterface);
	Rocket::Core::SetSystemInterface(&g_rocketInterface);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	rocket_interface_configuration rocketCFG;

	rocketCFG.blendDesc                              = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	rocketCFG.blendDesc.RenderTarget[0].BlendEnable  = TRUE;
	rocketCFG.blendDesc.RenderTarget[0].SrcBlend     = D3D12_BLEND_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].DestBlend    = D3D12_BLEND_INV_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].BlendOp      = D3D12_BLEND_OP_ADD;
	rocketCFG.blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;
	rocketCFG.rtvFormat                              = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	Rocket::Core::FontDatabase::LoadFontFace("ui/QuattrocentoSans-Regular.ttf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/QuattrocentoSans-Bold.ttf");

	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Roman.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Bold.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Italic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-BoldItalic.otf");

#endif

	debug_renderer_configuration debugRendererCFG;

	debugRendererCFG.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	debugRendererCFG.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	alpha_composer_configuration composerCFG;

	composerCFG.blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	composerCFG.blendDesc.RenderTarget[0].BlendEnable  = TRUE;
	composerCFG.blendDesc.RenderTarget[0].SrcBlend     = D3D12_BLEND_SRC_ALPHA;
	composerCFG.blendDesc.RenderTarget[0].DestBlend    = D3D12_BLEND_INV_SRC_ALPHA;
	composerCFG.blendDesc.RenderTarget[0].BlendOp      = D3D12_BLEND_OP_ADD;
	composerCFG.blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;

	composerCFG.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (g_realtimeRenderer.init(m_renderContext, &g_renderConfiguration) &&
		g_visualDebugger.init(device, debugRendererCFG) &&
	    g_tonemapper.init(device) &&
	    g_textRenderer.init(device) &&
	    g_alphaComposer.init(device, composerCFG))
	{

		/* Wait for the initialization operations to be done on the GPU
		   before starting the rendering and resetting the command allocator */

		FUSE_HR_CHECK(commandList->Close());
		commandQueue.execute(commandList);

		com_ptr<ID3D12Fence> initFence;
		HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&initFence));

		commandQueue->Signal(initFence.get(), 1u);
		initFence->SetEventOnCompletion(1u, hEvent);

		g_initialized = WAIT_OBJECT_0 == WaitForSingleObject(hEvent, INFINITE);

		CloseHandle(hEvent);

#ifdef FUSE_USE_EDITOR_GUI
		FAIL_IF(!g_wxEditorGUI.init(get_wx_window(), &g_scene, &g_renderConfiguration, &g_visualDebugger));
		load_ui_configuration(FUSE_UI_CONFIGURATION);
#endif

		return g_initialized;

	}

	return false;

}

void renderer_application::on_render_context_released(gpu_render_context & renderContext)
{

	g_renderResourceManager.clear();

	g_imageManager.reset();
	g_meshManager.reset();
	g_gpuMeshManager.reset();

	g_resourceFactory.clear();

#ifdef FUSE_USE_LIBROCKET
	Rocket::Core::Shutdown();
	g_rocketInterface.shutdown();
#endif

#ifdef defined(FUSE_USE_EDITOR_GUI) && defined(FUSE_WXWIDGETS)
	if (g_initialized)
	{
		save_ui_configuration(FUSE_UI_CONFIGURATION);
	}

	g_wxEditorGUI.shutdown();
#endif
}

bool renderer_application::on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc)
{
	get_command_queue().wait_for_frame(get_command_queue().get_frame_index());

	g_renderConfiguration.set_render_resolution(uint2(desc->Width, desc->Height));

	g_fullscreenViewport.Width    = desc->Width;
	g_fullscreenViewport.Height   = desc->Height;
	g_fullscreenViewport.MaxDepth = 1.f;

	g_fullscreenScissorRect.right  = desc->Width;
	g_fullscreenScissorRect.bottom = desc->Height;

	g_cameraController.on_resize(desc->Width, desc->Height);

	FAIL_IF (!g_sdsm.init(device, desc->Width, desc->Height))

	camera * mainCamera = g_scene.get_active_camera();

	if (mainCamera)
	{
		mainCamera->set_aspect_ratio((float) desc->Width / desc->Height);
	}

#ifdef FUSE_USE_LIBROCKET
	g_rocketInterface.on_resize(desc->Width, desc->Height);
#endif

	return true;
}

bool renderer_application::on_mouse_event(const mouse & mouse, const mouse_event_info & event)
{
	if (event.type == FUSE_MOUSE_EVENT_WHEEL)
	{	
		//g_cameraController.set_speed(to_float3(vec128Scale(to_vector(g_cameraController.get_speed()), (event.wheel * .2f + 1.f))));
		g_cameraController.set_speed(g_cameraController.get_speed() * (event.wheel * .2f + 1.f));
		return true;
	}
	else if (event.type == FUSE_MOUSE_EVENT_BUTTON_DOWN)
	{
#ifdef FUSE_USE_EDITOR_GUI
		if (event.key == FUSE_MOUSE_VK_MOUSE1)
		{
			/*camera * cam = g_scene.get_active_camera();

			float4x4 vp = cam->get_view_matrix() * cam->get_projection_matrix();
			float4x4 invVP = inverse(vp);

			float2 screenPosition = {
				(float)event.position.x / get_render_window_width(),
				(float)event.position.y / get_render_window_height()
			};

			screenPosition = 2.f * screenPosition - 1.f;

			float4 clipPosition = float4(screenPosition.x, -screenPosition.y, 1, 1) * invVP;
			float3 camPosition = cam->get_position();
			float3 farPosition = to_float3(clipPosition);

			ray r;
			r.set_origin(to_vec128(camPosition));
			r.set_direction(to_vec128(normalize(farPosition - camPosition)));*/

			camera * c = g_scene.get_active_camera();

			float4x4 viewProjMatrix    = c->get_view_matrix() * c->get_projection_matrix();
			float4x4 invViewProjMatrix = inverse(viewProjMatrix);

			float2 deviceCoords = {
				(2 * event.position.x) / (float) get_render_window_width() - 1.f,
				1.f - (2 * event.position.y) / (float) get_render_window_height()
			};

			float4 nearH = float4(deviceCoords.x, deviceCoords.y, 0, 1) * invViewProjMatrix;
			float4 farH  = float4(deviceCoords.x, deviceCoords.y, 1, 1) * invViewProjMatrix;

			float3 n = to_float3(nearH) / nearH.w;
			float3 f = to_float3(farH) / farH.w;

			ray r;
			r.set_origin(to_vec128(c->get_position()));
			r.set_direction(to_vec128(normalize(f - n)));

			scene_graph_geometry * node;
			float t;

			if (g_scene.ray_pick(r, node, t))
			{
				g_wxEditorGUI.set_selected_node(node);
			}
			else
			{
				g_wxEditorGUI.set_selected_node(g_scene.get_scene_graph()->get_root());
			}

			//g_visualDebugger.add_persistent(FUSE_LITERAL("casted_ray"), r, color_rgba::green);
		}
#endif
	}

	return false;
}

bool renderer_application::on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event)
{

	if (event.type == FUSE_KEYBOARD_EVENT_BUTTON_DOWN)
	{

		switch (event.key)
		{

		case FUSE_KEYBOARD_VK_F11:
		{

			bool goFullscreen = !is_fullscreen();

			set_fullscreen(goFullscreen);
			set_cursor(goFullscreen, goFullscreen);

			g_cameraController.set_auto_center_mouse(goFullscreen);

			return true;

		}

		}

	}

	return false;

}

void renderer_application::on_update(float dt)
{
	g_cameraController.on_update(dt);
	g_scene.update();
}

void renderer_application::upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, ID3D12Resource * cbPerFrameBuffer)
{
	/* Setup per frame constant buffer */

	camera * camera = g_scene.get_active_camera();

	auto view              = to_mat128(camera->get_view_matrix());
	auto projection        = to_mat128(camera->get_projection_matrix());
	auto viewProjection    = view * projection;
	auto invViewProjection = mat128_inverse4(viewProjection);

	cb_per_frame cbPerFrame;

	/* Camera */

	cbPerFrame.camera.position          = reinterpret_cast<const float3&>(camera->get_position());
	cbPerFrame.camera.fovy              = camera->get_fovy();
	cbPerFrame.camera.aspectRatio       = camera->get_aspect_ratio();
	cbPerFrame.camera.znear             = camera->get_znear();
	cbPerFrame.camera.zfar              = camera->get_zfar();
	cbPerFrame.camera.view              = view;
	cbPerFrame.camera.projection        = projection;
	cbPerFrame.camera.viewProjection    = viewProjection;
	cbPerFrame.camera.invViewProjection = invViewProjection;

	/* Screen settings */

	cbPerFrame.screen.resolution      = uint2(get_render_window_width(), get_render_window_height());
	cbPerFrame.screen.orthoProjection = to_mat128(ortho_lh(0, get_render_window_width(), get_render_window_height(), 0, 0, 1));

	/* Render variables */

	cbPerFrame.rvars.shadowMapResolution = g_renderConfiguration.get_shadow_map_resolution();
	cbPerFrame.rvars.vsmMinVariance      = g_renderConfiguration.get_vsm_min_variance();
	cbPerFrame.rvars.vsmMinBleeding      = g_renderConfiguration.get_vsm_min_bleeding();

	cbPerFrame.rvars.evsm2MinVariance    = g_renderConfiguration.get_evsm2_min_variance();
	cbPerFrame.rvars.evsm2MinBleeding    = g_renderConfiguration.get_evsm2_min_bleeding();
	cbPerFrame.rvars.evsm2Exponent       = esm_clamp_exponent(g_renderConfiguration.get_evsm2_exponent(), g_renderConfiguration.get_evsm2_float_precision());

	cbPerFrame.rvars.evsm4MinVariance    = g_renderConfiguration.get_evsm4_min_variance();
	cbPerFrame.rvars.evsm4MinBleeding    = g_renderConfiguration.get_evsm4_min_bleeding();
	cbPerFrame.rvars.evsm4PosExponent    = esm_clamp_exponent_positive(g_renderConfiguration.get_evsm4_positive_exponent(), g_renderConfiguration.get_evsm4_float_precision());
	cbPerFrame.rvars.evsm4NegExponent    = esm_clamp_exponent_positive(g_renderConfiguration.get_evsm4_negative_exponent(), g_renderConfiguration.get_evsm4_float_precision());

	/* Upload */

	D3D12_GPU_VIRTUAL_ADDRESS address;
	D3D12_GPU_VIRTUAL_ADDRESS heapAddress = ringBuffer.get_heap()->GetGPUVirtualAddress();

	void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_frame), &address);

	memcpy(cbData, &cbPerFrame, sizeof(cb_per_frame));

	gpu_upload_buffer(commandQueue, commandList, cbPerFrameBuffer, 0, ringBuffer.get_heap(), address - heapAddress, sizeof(cb_per_frame));
}

void draw_bounding_volume(scene_graph_geometry * node)
{
	static const color_rgba debugColors[] = {
		{ 1, 0, 0, 1 },
		{ 0, 1, 0, 1 },
		{ 0, 0, 1, 1 },
		{ 0, 1, 1, 1 },
		{ 1, 0, 1, 1 },
		{ 1, 1, 0, 1 }
	};

	int colorIndex = node->get_gpu_mesh()->get_id() % _countof(debugColors);

	g_visualDebugger.add(
		bounding_aabb(node->get_global_bounding_sphere()),
		debugColors[colorIndex]);
}

void renderer_application::on_render(gpu_render_context & renderContext, const render_resource & backBuffer)
{
	ID3D12Device * device = renderContext.get_device();

	uint32_t bufferIndex  = renderContext.get_buffer_index();

	auto & commandQueue   = renderContext.get_command_queue();

	auto & commandList    = renderContext.get_command_list();
	auto & guiCommandList = g_guiCommandList[bufferIndex];

	g_realtimeRenderer.update_render_configuration();

	uint2 renderResolution = g_renderConfiguration.get_render_resolution();

	/* Reset command lists */

	renderContext.reset_command_allocators();
	commandList.reset_command_list(nullptr);

	guiCommandList.reset_command_allocator();
	guiCommandList.reset_command_list(nullptr);

	/* Setup render */

	commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());
	guiCommandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	upload_per_frame_resources(device, commandQueue, commandList, renderContext.get_ring_buffer(), g_cbPerFrame[bufferIndex].get());
	commandList.resource_barrier_transition(g_cbPerFrame[bufferIndex].get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	
	/* Draw GUI */

	render_resource_ptr guiRenderTarget = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		renderResolution.x, renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, color_rgba::zero));

	guiCommandList.resource_barrier_transition(guiRenderTarget->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	guiCommandList->ClearRenderTargetView(guiRenderTarget->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);

	draw_gui(device, commandQueue, guiCommandList, renderContext.get_ring_buffer(), *guiRenderTarget.get());

	FUSE_HR_CHECK(guiCommandList->Close());
	commandQueue.execute(guiCommandList);

	/* Render */

	g_realtimeRenderer.render(cbPerFrameAddress, &g_scene, g_scene.get_active_camera());
	g_realtimeRenderer.get_culling_results();

	/* Tonemap */

	const render_resource * hdrRenderTarget = g_realtimeRenderer.get_final_resource();

	render_resource_ptr ldrRenderTarget = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM,
		renderResolution.x, renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, color_rgba::zero));

	g_tonemapper.render(
		device,
		commandQueue,
		commandList,
		*hdrRenderTarget,
		*ldrRenderTarget.get(),
		renderResolution.x,
		renderResolution.y);

	if (g_visualDebugger.get_draw_bounding_volumes())
	{
		for (auto * g : g_realtimeRenderer.get_culling_results())
		{
			draw_bounding_volume(g);
		}
	}
#ifdef FUSE_USE_EDITOR_GUI
	else if (g_visualDebugger.get_draw_selected_node_bounding_volume())
	{
		scene_graph_node * selectedNode = g_wxEditorGUI.get_selected_node();

		if (selectedNode->get_type() == FUSE_SCENE_GRAPH_GEOMETRY)
		{
			draw_bounding_volume(static_cast<scene_graph_geometry*>(selectedNode));
		}
	}
#endif

	if (g_visualDebugger.get_draw_octree())
	{
		g_scene.draw_octree(&g_visualDebugger);
	}

	g_visualDebugger.render(
		device,
		commandQueue,
		commandList,
		renderContext.get_ring_buffer(),
		cbPerFrameAddress,
		*ldrRenderTarget.get(),
		*g_realtimeRenderer.get_depth_resource());

	/* Finally copy to backbuffer */

	commandList.resource_barrier_transition(ldrRenderTarget->get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList.resource_barrier_transition(backBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(backBuffer.get(), ldrRenderTarget->get());

	/* Compose */

	const render_resource * composeResources[] = {
		guiRenderTarget.get()
	};

	commandList->RSSetViewports(1, &g_fullscreenViewport);
	commandList->RSSetScissorRects(1, &g_fullscreenScissorRect);

	g_alphaComposer.render(
		commandQueue,
		commandList,
		backBuffer,
		composeResources,
		_countof(composeResources));

	/* Present */

	commandList.resource_barrier_transition(backBuffer.get(), D3D12_RESOURCE_STATE_PRESENT);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);
}

void renderer_application::draw_gui(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, const render_resource & renderTarget)
{
	commandList->RSSetViewports(1, &g_fullscreenViewport);
	commandList->RSSetScissorRects(1, &g_fullscreenScissorRect);

	uint32_t bufferIndex = get_buffer_index();

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	auto guiRTV = renderTarget.get_rtv_cpu_descriptor_handle();

	std::stringstream guiSS;

	guiSS << "FPS: " << get_fps() << std::endl;

	g_textRenderer.render(
		device,
		commandQueue,
		commandList,
		ringBuffer,
		renderTarget.get(),
		&guiRTV,
		cbPerFrameAddress,
		g_font.get(),
		guiSS.str().c_str(),
		float2(16, 16),
		float4(.65f, .2f, .25f, 1));
}

void renderer_application::on_configuration_init(application_config * configuration)
{
	configuration->syncInterval         = 0;
	configuration->presentFlags         = 0;
	configuration->swapChainBufferCount = NUM_BUFFERS;
	configuration->swapChainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM;
	configuration->uploadHeapSize       = UPLOAD_HEAP_SIZE;
}