#include "renderer_application.hpp"

#include <fuse/logger.hpp>

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
#include "deferred_renderer.hpp"
#include "scene.hpp"
#include "shadow_mapper.hpp"
#include "tonemapper.hpp"
#include "editor_gui.hpp"
#include "render_variables.hpp"
#include "blur.hpp"
#include "sdsm.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define FAIL_IF(Condition) { if (Condition) return false; }

#define MAX_DSV 8
#define MAX_RTV 1024
#define MAX_SRV 1024

#define SKYBOX_RESOLUTION 128

#define NUM_BUFFERS      (2u)
#define UPLOAD_HEAP_SIZE (16 << 20)

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

rocket_interface   g_rocketInterface;

bitmap_font_ptr    g_font;

scene              g_scene;

debug_renderer     g_debugRenderer;
deferred_renderer  g_deferredRenderer;
text_renderer      g_textRenderer;
tonemapper         g_tonemapper;
shadow_mapper      g_shadowMapper;
sdsm               g_sdsm;

renderer_configuration g_rendererConfiguration;

blur           g_shadowMapBlur;
alpha_composer g_alphaComposer;

skybox_renderer g_skyboxRenderer;

D3D12_VIEWPORT g_fullscreenViewport;
D3D12_RECT     g_fullscreenScissorRect;

DXGI_FORMAT g_shadowMapRTV;

#ifdef FUSE_USE_EDITOR_GUI

editor_gui g_editorGUI;
bool       g_showGUI = true;

#endif

bool renderer_application::on_render_context_created(gpu_render_context & renderContext)
{

	ID3D12Device * device       = renderContext.get_device();
	auto         & commandQueue = renderContext.get_command_queue();
	auto         & commandList  = renderContext.get_command_list();

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
		!g_scene.get_skybox()->init(device, SKYBOX_RESOLUTION, NUM_BUFFERS) ||
		!g_skyboxRenderer.init(device)
	)

	g_scene.get_skybox()->set_zenith(1.02539599f);
	g_scene.get_skybox()->set_azimuth(-1.70169306f);

	g_scene.set_scene_bounds(XMVectorZero(), 300000.f);
	g_scene.recalculate_octree();
	
	/*auto light = *g_scene.get_lights().first;

	light->direction = XMFLOAT3(-0.281581f, 0.522937f, -0.804518f);
	light->intensity *= .2f;*/
	//g_scene.get_active_camera()->set_zfar(10000.f);

	//g_cameraController = std::make_unique<fps_camera_controller>(g_scene.get_active_camera());

	g_cameraController.set_camera(g_scene.get_active_camera());
	g_cameraController.set_speed(XMFLOAT3(200, 200, 200));
	
	add_keyboard_listener(&g_cameraController);
	add_mouse_listener(&g_cameraController);

	g_scene.get_active_camera()->set_orientation(XMQuaternionIdentity());

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
	rendererCFG.shadowMappingAlgorithm = g_rendererConfiguration.get_shadow_mapping_algorithm();

	FAIL_IF(!create_command_lists(g_guiCommandList.begin(), g_guiCommandList.end(), device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, nullptr));

	for (auto & commandList : g_guiCommandList) FAIL_IF(FUSE_HR_FAILED(commandList->Close()));

#ifdef FUSE_USE_EDITOR_GUI

	Rocket::Core::SetRenderInterface(&g_rocketInterface);
	Rocket::Core::SetSystemInterface(&g_rocketInterface);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	Rocket::Core::FontDatabase::LoadFontFace("ui/QuattrocentoSans-Regular.ttf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/QuattrocentoSans-Bold.ttf");

	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Roman.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Bold.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-Italic.otf");
	Rocket::Core::FontDatabase::LoadFontFace("ui/demo/Delicious-BoldItalic.otf");

	rocket_interface_configuration rocketCFG;

	rocketCFG.blendDesc                              = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	rocketCFG.blendDesc.RenderTarget[0].BlendEnable  = TRUE;
	rocketCFG.blendDesc.RenderTarget[0].SrcBlend     = D3D12_BLEND_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].DestBlend    = D3D12_BLEND_INV_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].BlendOp      = D3D12_BLEND_OP_ADD;
	rocketCFG.blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;

	rocketCFG.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	FAIL_IF (
	    !g_rocketInterface.init(device, rocketCFG) ||
		!g_editorGUI.init(&g_scene, &g_rendererConfiguration)
	)

	g_editorGUI.set_render_options_visibility(true);
	//g_editorGUI.set_light_panel_visibility(true);
	g_editorGUI.set_skybox_panel_visibility(true);

	add_mouse_listener(&g_editorGUI, FUSE_PRIORITY_DEFAULT_DELTA(1));
	add_keyboard_listener(&g_editorGUI, FUSE_PRIORITY_DEFAULT_DELTA(1));

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

	if (g_deferredRenderer.init(device, rendererCFG) &&
	    g_debugRenderer.init(device, debugRendererCFG) &&
	    g_tonemapper.init(device) &&
	    g_textRenderer.init(device) &&
	    g_alphaComposer.init(device, composerCFG) &&
	    create_shadow_map_resources(device))
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

		bool result = WAIT_OBJECT_0 == WaitForSingleObject(hEvent, INFINITE);

		CloseHandle(hEvent);

		return result;

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

#if FUSE_USE_EDITOR_GUI

	g_editorGUI.shutdown();
	Rocket::Core::Shutdown();
	g_rocketInterface.shutdown();

#endif

}

bool renderer_application::on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc)
{

	g_renderResourceManager.clear();

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

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_depthBuffer[i];

	//	r.clear();

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_D24_UNORM_S8_UINT,
	//				desc->Width, desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0)
	//		)  ||
	//		!r.create_depth_stencil_view(device)
	//	)

	//	r->SetName(L"main_depth_buffer");

	//}

	//const float black[4] = { 0 };

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresRGBA16F0[i];
	//	
	//	r.clear();

	//	// gbuffer 0
	//	r.set_srv_token(g_gbufferSRVTable[i]);

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R16G16B16A16_FLOAT,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
	//		)  ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device)
	//	)

	//	r->SetName(L"fullres_rgba16f0");
	//	
	//}

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresRGBA16F1[i];

	//	r.clear();

	//	// gbuffer 1
	//	r.set_srv_token(g_gbufferSRVTable[i] + 1);

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R16G16B16A16_FLOAT,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
	//		)  ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device)
	//	)

	//	r->SetName(L"fullres_rgba16f1");

	//}

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresRGBA16F2[i];

	//	r.clear();

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R16G16B16A16_FLOAT,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
	//		) ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device)
	//	)

	//	r->SetName(L"fullres_rgba16f2");

	//}

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresRGBA8U0[i];

	//	r.clear();

	//	// gbuffer 2
	//	r.set_srv_token(g_gbufferSRVTable[i] + 2);

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R8G8B8A8_UNORM,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, black)
	//		) ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device) ||
	//		!r.create_unordered_access_view(device)
	//	)

	//	r->SetName(L"fullres_rgba8u0");

	//}

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresRGBA8U1[i];

	//	r.clear();

	//	// gbuffer 3
	//	r.set_srv_token(g_gbufferSRVTable[i] + 3);

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R16G16B16A16_FLOAT,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
	//			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
	//		) ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device)
	//	)

	//	r->SetName(L"fullres_rgba8u1");

	//}

	//for (int i = 0; i < NUM_BUFFERS; i++)
	//{

	//	auto & r = g_fullresGUI[i];

	//	r.clear();

	//	FAIL_IF (
	//		!r.create(
	//			device,
	//			&CD3DX12_RESOURCE_DESC::Tex2D(
	//				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	//				desc->Width,
	//				desc->Height,
	//				1, 1, 1, 0,
	//				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
	//			D3D12_RESOURCE_STATE_RENDER_TARGET,
	//			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, black)
	//		) ||
	//		!r.create_render_target_view(device) ||
	//		!r.create_shader_resource_view(device)
	//	)

	//	r->SetName(L"fullres_gui");

	//}

#ifdef FUSE_USE_EDITOR_GUI
	g_rocketInterface.on_resize(desc->Width, desc->Height);
	g_editorGUI.on_resize(desc->Width, desc->Height);
#endif

	return true;

}

bool renderer_application::on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event)
{

	if (event.type == FUSE_KEYBOARD_EVENT_BUTTON_DOWN)
	{

		switch (event.key)
		{

		case FUSE_KEYBOARD_VK_F4:
			g_showGUI = !g_showGUI;
			return true;

		case FUSE_KEYBOARD_VK_F9:
			g_editorGUI.set_debugger_visibility(!g_editorGUI.get_debugger_visibility());
			return true;

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

#if FUSE_USE_EDITOR_GUI

	if (g_showGUI)
	{
		g_editorGUI.update();
	}

#endif

}

void renderer_application::update_renderer_configuration(ID3D12Device * device, gpu_command_queue & commandQueue)
{

	auto updates = g_rendererConfiguration.get_updates();

	for (auto variable : updates)
	{

		switch (variable)
		{

		case FUSE_RVAR_VSM_FLOAT_PRECISION:
		case FUSE_RVAR_EVSM2_FLOAT_PRECISION:
		case FUSE_RVAR_EVSM4_FLOAT_PRECISION:
		case FUSE_RVAR_SHADOW_MAPPING_ALGORITHM:
			commandQueue.wait_for_frame(commandQueue.get_frame_index());
			create_shadow_map_resources(device);
			g_deferredRenderer.set_shadow_mapping_algorithm(g_rendererConfiguration.get_shadow_mapping_algorithm());
			break;

		case FUSE_RVAR_SHADOW_MAP_RESOLUTION:
			commandQueue.wait_for_frame(commandQueue.get_frame_index());
			create_shadow_map_resources(device);
			break;

		case FUSE_RVAR_VSM_BLUR_KERNEL_SIZE:
		case FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE:
		case FUSE_RVAR_EVSM4_BLUR_KERNEL_SIZE:
			create_shadow_map_resources(device, false);
			break;

		}

	}

}

void renderer_application::upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, ID3D12Resource * cbPerFrameBuffer)
{

	/* Setup per frame constant buffer */

	camera * camera = g_scene.get_active_camera();

	auto view              = camera->get_view_matrix();
	auto projection        = camera->get_projection_matrix();
	auto viewProjection    = XMMatrixMultiply(view, projection);
	auto invViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProjection), viewProjection);

	cb_per_frame cbPerFrame;

	/* Camera */

	cbPerFrame.camera.position          = to_float3(camera->get_position());
	cbPerFrame.camera.fovy              = camera->get_fovy();
	cbPerFrame.camera.aspectRatio       = camera->get_aspect_ratio();
	cbPerFrame.camera.znear             = camera->get_znear();
	cbPerFrame.camera.zfar              = camera->get_zfar();
	cbPerFrame.camera.view              = XMMatrixTranspose(view);
	cbPerFrame.camera.projection        = XMMatrixTranspose(projection);
	cbPerFrame.camera.viewProjection    = XMMatrixTranspose(viewProjection);
	cbPerFrame.camera.invViewProjection = XMMatrixTranspose(invViewProjection);

	/* Screen settings */

	cbPerFrame.screen.resolution      = XMUINT2(get_render_window_width(), get_render_window_height());
	cbPerFrame.screen.orthoProjection = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0, get_render_window_width(), get_render_window_height(), 0, 0, 1));

	/* Render variables */

	cbPerFrame.rvars.shadowMapResolution = g_rendererConfiguration.get_shadow_map_resolution();
	cbPerFrame.rvars.vsmMinVariance      = g_rendererConfiguration.get_vsm_min_variance();
	cbPerFrame.rvars.vsmMinBleeding      = g_rendererConfiguration.get_vsm_min_bleeding();

	cbPerFrame.rvars.evsm2MinVariance    = g_rendererConfiguration.get_evsm2_min_variance();
	cbPerFrame.rvars.evsm2MinBleeding    = g_rendererConfiguration.get_evsm2_min_bleeding();
	cbPerFrame.rvars.evsm2Exponent       = esm_clamp_exponent(g_rendererConfiguration.get_evsm2_exponent(), g_rendererConfiguration.get_evsm2_float_precision());

	cbPerFrame.rvars.evsm4MinVariance    = g_rendererConfiguration.get_evsm4_min_variance();
	cbPerFrame.rvars.evsm4MinBleeding    = g_rendererConfiguration.get_evsm4_min_bleeding();
	cbPerFrame.rvars.evsm4PosExponent    = esm_clamp_exponent_positive(g_rendererConfiguration.get_evsm4_positive_exponent(), g_rendererConfiguration.get_evsm4_float_precision());
	cbPerFrame.rvars.evsm4NegExponent    = esm_clamp_exponent_positive(g_rendererConfiguration.get_evsm4_negative_exponent(), g_rendererConfiguration.get_evsm4_float_precision());

	/* Upload */

	D3D12_GPU_VIRTUAL_ADDRESS address;
	D3D12_GPU_VIRTUAL_ADDRESS heapAddress = ringBuffer.get_heap()->GetGPUVirtualAddress();

	void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_frame), &address);

	memcpy(cbData, &cbPerFrame, sizeof(cb_per_frame));

	gpu_upload_buffer(commandQueue, commandList, cbPerFrameBuffer, 0, ringBuffer.get_heap(), address - heapAddress, sizeof(cb_per_frame));

}

void renderer_application::on_render(gpu_render_context & renderContext, const render_resource & backBuffer)
{

	ID3D12Device * device = renderContext.get_device();

	uint32_t bufferIndex  = renderContext.get_buffer_index();

	auto & commandQueue   = renderContext.get_command_queue();

	auto & commandList    = renderContext.get_command_list();
	auto & guiCommandList = g_guiCommandList[bufferIndex];

	update_renderer_configuration(device, commandQueue);

	/* Reset command list */

	renderContext.reset_command_allocators();
	commandList.reset_command_list(nullptr);

	guiCommandList.reset_command_allocator();
	guiCommandList.reset_command_list(nullptr);

	/* Setup render */

	commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());
	guiCommandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	upload_per_frame_resources(device, commandQueue, commandList, renderContext.get_ring_buffer(), g_cbPerFrame[bufferIndex].get());

	auto shadowMapViewport    = make_fullscreen_viewport(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution());
	auto shadowMapScissorRect = make_fullscreen_scissor_rect(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution());

	/* Get the resources for gbuffer rendering and clear them */

	float clearColor[4] = { 0.f };

	render_resource_ptr gbufferResources[] = {

		g_renderResourceManager.get_texture_2d(
			device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
			get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor)),

		g_renderResourceManager.get_texture_2d(
			device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
			get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor)),

		g_renderResourceManager.get_texture_2d(
			device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM,
			get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor)),

		g_renderResourceManager.get_texture_2d(
			device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
			get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor))

	};

	for (int i = 0; i < 4; i++)
	{

		device->CreateShaderResourceView(
			gbufferResources[i]->get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
				cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(g_gbufferSRVTable[bufferIndex]),
				i,
				cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_descriptor_size()));

	}

	const render_resource * gbuffer[] = {
		gbufferResources[0].get(),
		gbufferResources[1].get(),
		gbufferResources[2].get(),
		gbufferResources[3].get()
	};

	render_resource_ptr sceneDepthBuffer = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_D24_UNORM_S8_UINT,
		get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0));

	/*render_resource * gbuffer[] = {
		std::addressof(g_fullresRGBA16F0[bufferIndex]),
		std::addressof(g_fullresRGBA16F1[bufferIndex]),
		std::addressof(g_fullresRGBA8U0[bufferIndex]),
		std::addressof(g_fullresRGBA8U1[bufferIndex]),
		std::addressof(g_depthBuffer[bufferIndex])
	};*/

	//auto dsvHandle = g_depthBuffer[bufferIndex].get_dsv_cpu_descriptor_handle();

	auto dsvHandle = sceneDepthBuffer->get_dsv_cpu_descriptor_handle();

	commandList->ClearRenderTargetView(gbuffer[0]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[1]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[2]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[3]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(sceneDepthBuffer->get_dsv_cpu_descriptor_handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

	commandList.resource_barrier_transition(g_cbPerFrame[bufferIndex].get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	/* Draw GUI */

	render_resource_ptr guiRenderTarget = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, clearColor));

	guiCommandList.resource_barrier_transition(guiRenderTarget->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	guiCommandList->ClearRenderTargetView(guiRenderTarget->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	draw_gui(device, commandQueue, guiCommandList, renderContext.get_ring_buffer(), *guiRenderTarget.get());

	FUSE_HR_CHECK(guiCommandList->Close());
	commandQueue.execute(guiCommandList);

	/* GBuffer rendering */

	auto culledObjects = g_scene.frustum_culling(g_scene.get_active_camera()->get_frustum());

	auto renderableObjects = std::make_pair(culledObjects.begin(), culledObjects.end());
	//auto renderableObjects = g_scene.get_renderable_objects();

	commandList->RSSetViewports(1, &g_fullscreenViewport);
	commandList->RSSetScissorRects(1, &g_fullscreenScissorRect);

	g_deferredRenderer.render_gbuffer(
		device,
		commandQueue,
		commandList,
		renderContext.get_ring_buffer(),
		cbPerFrameAddress,
		gbuffer,
		//g_depthBuffer[bufferIndex],
		*sceneDepthBuffer.get(),
		g_scene.get_active_camera(),
		renderableObjects.first,
		renderableObjects.second);

	g_sdsm.create_log_partitions(device, commandQueue, commandList, *sceneDepthBuffer.get(), g_sdsmConstantBuffer[bufferIndex]);

	/* Prepare for shading */

	render_resource_ptr hdrRenderTarget = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
		get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, clearColor));

	commandList.resource_barrier_transition(hdrRenderTarget->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(hdrRenderTarget->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	/* Shading */

	/*auto lightsIterators = g_scene.get_lights();

	for (auto it = lightsIterators.first; it != lightsIterators.second; it++)
	{

		light * currentLight = *it;

		shadow_map_info shadowMapInfo, * pShadowMapInfo = nullptr;

		if (currentLight->type == FUSE_LIGHT_TYPE_DIRECTIONAL)
		{

			shadowMapInfo.shadowMap   = std::addressof(g_shadowmap0[bufferIndex]);
			shadowMapInfo.lightMatrix = sm_directional_light_matrix(currentLight->direction, staticObjects.first, staticObjects.second);

			g_shadowMapper.render(
				device,
				commandQueue,
				commandList,
				renderContext.get_ring_buffer(),
				cbPerFrameAddress,
				shadowMapInfo.lightMatrix,
				g_shadowmap0[bufferIndex],
				g_shadowmap32F[bufferIndex],
				staticObjects.first,
				staticObjects.second);

			g_shadowMapBlur.render(
				commandQueue,
				commandList,
				g_shadowmap0[bufferIndex],
				g_shadowmap1[bufferIndex]);

			g_mipmapGenerator->generate_mipmaps(device, commandQueue, commandList, g_shadowmap0[bufferIndex].get());

			pShadowMapInfo = &shadowMapInfo;

		}

		g_deferredRenderer.render_light(
			device,
			commandQueue,
			commandList,
			renderContext.get_ring_buffer(),
			cbPerFrameAddress,
			g_fullresRGBA16F2[bufferIndex],
			gbuffer,
			currentLight,
			pShadowMapInfo);

	}*/

	/* Skybox and skylight */

	render_resource_ptr shadowMapResources[] = {
		get_shadow_map_render_target(),
		get_shadow_map_render_target()
	};

	D3D12_DEPTH_STENCIL_VIEW_DESC shadowMapDSVDesc = {};

	shadowMapDSVDesc.Format        = DXGI_FORMAT_D32_FLOAT;
	shadowMapDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	render_resource_ptr shadowMapDepth = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R32_TYPELESS,
		g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution(), 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.f, 0),
		nullptr, nullptr, nullptr,
		&shadowMapDSVDesc);

	{

		shadow_map_info shadowMapInfo = {};

		shadowMapInfo.sdsm          = true;
		shadowMapInfo.sdsmCBAddress = g_sdsmConstantBuffer[bufferIndex]->GetGPUVirtualAddress();

		skybox * skybox = g_scene.get_skybox();

		g_skyboxRenderer.render(device, commandQueue, commandList, renderContext.get_ring_buffer(), *skybox);

		light sunLight = skybox->get_sun_light();

		shadowMapInfo.shadowMap = shadowMapResources[0].get();

		{

			XMMATRIX lightViewMatrix = XMMatrixLookAtLH(XMVectorZero(),
			                                            to_vector(sunLight.direction),
			                                            g_scene.get_active_camera()->left());
			                                            //XMVectorSet(sunLight.direction.y > .99f ? 1.f : 0.f, sunLight.direction.y > .99f ? 0.f : 1.f, 0.f, 0.f));

			XMMATRIX lightCropMatrix = sm_crop_matrix_lh(lightViewMatrix, renderableObjects.first, renderableObjects.second);

			shadowMapInfo.lightMatrix = XMMatrixMultiply(lightViewMatrix, lightCropMatrix);

		}

		/* Sun shadow map */

		commandList->RSSetViewports(1, &shadowMapViewport);
		commandList->RSSetScissorRects(1, &shadowMapScissorRect);

		g_shadowMapper.render(
			device,
			commandQueue,
			commandList,
			renderContext.get_ring_buffer(),
			cbPerFrameAddress,
			shadowMapInfo.lightMatrix,
			*shadowMapResources[0].get(),
			*shadowMapDepth.get(),
			renderableObjects.first,
			renderableObjects.second);

		g_shadowMapBlur.render(
			commandQueue,
			commandList,
			*shadowMapResources[0].get(),
			*shadowMapResources[1].get());

		g_mipmapGenerator->generate_mipmaps(device, commandQueue, commandList, shadowMapResources[0]->get());

		/* Skylight rendering */

		commandList->RSSetViewports(1, &g_fullscreenViewport);
		commandList->RSSetScissorRects(1, &g_fullscreenScissorRect);

		g_deferredRenderer.render_light(
			device,
			commandQueue,
			commandList,
			renderContext.get_ring_buffer(),
			cbPerFrameAddress,
			*hdrRenderTarget.get(),
			gbuffer,
			cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(g_gbufferSRVTable[bufferIndex]),
			&sunLight,
			&shadowMapInfo);
		
		/* Skybox rendering */

		g_deferredRenderer.render_skybox(
			device,
			commandQueue,
			commandList,
			cbPerFrameAddress,
			*hdrRenderTarget.get(),
			//g_depthBuffer[bufferIndex],
			*sceneDepthBuffer.get(),
			*skybox);

	}	

	/* Tonemap */

	render_resource_ptr ldrRenderTarget = g_renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM,
		get_render_window_width(), get_render_window_height(), 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor));

	g_tonemapper.render(
		device,
		commandQueue,
		commandList,
		*hdrRenderTarget.get(),
		*ldrRenderTarget.get(),
		get_render_window_width(),
		get_render_window_height());

	color_rgba debugColors[] = {
		{ 1, 0, 0, 1 },
		{ 0, 1, 0, 1 },
		{ 0, 0, 1, 1 },
		{ 0, 1, 1, 1 },
		{ 1, 0, 1, 1 },
		{ 1, 1, 0, 1 }
	};

	for (auto it = renderableObjects.first; it != renderableObjects.second; it++)
	{

		int colorIndex = (*it)->get_mesh()->get_id() % _countof(debugColors);
		
		g_debugRenderer.add_aabb(
			bounding_aabb(transform_affine((*it)->get_bounding_sphere(), (*it)->get_world_matrix())),
			debugColors[colorIndex]);

	}

	g_debugRenderer.render(
		device,
		commandQueue,
		commandList,
		renderContext.get_ring_buffer(),
		cbPerFrameAddress,
		*ldrRenderTarget.get(),
		*sceneDepthBuffer.get());

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
		XMFLOAT2(16, 16),
		XMFLOAT4(1, 1, 0, 1));

#ifdef FUSE_USE_EDITOR_GUI

	if (g_showGUI)
	{

		g_rocketInterface.render_begin(
			commandQueue,
			commandList,
			ringBuffer,
			cbPerFrameAddress,
			renderTarget.get(),
			guiRTV);

		g_editorGUI.render();

		g_rocketInterface.render_end();

	}

#endif

}

void renderer_application::on_configuration_init(application_config * configuration)
{
	configuration->syncInterval         = 0;
	configuration->presentFlags         = 0;
	configuration->swapChainBufferCount = NUM_BUFFERS;
	configuration->swapChainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM;
	configuration->uploadHeapSize       = UPLOAD_HEAP_SIZE;
}

bool renderer_application::create_shadow_map_resources(ID3D12Device * device, bool createBuffers)
{

	uint32_t resolution = g_rendererConfiguration.get_shadow_map_resolution();
	shadow_mapping_algorithm algorithm = g_rendererConfiguration.get_shadow_mapping_algorithm();

	switch (algorithm)
	{

	case FUSE_SHADOW_MAPPING_NONE:
		return true;

	case FUSE_SHADOW_MAPPING_VSM:

		if (g_rendererConfiguration.get_evsm2_float_precision() == 16)
		{
			g_shadowMapRTV = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			g_shadowMapRTV = DXGI_FORMAT_R32G32_FLOAT;
		}

		FAIL_IF(!g_shadowMapBlur.init_box_blur(
			device,
			"float2",
			g_rendererConfiguration.get_vsm_blur_kernel_size(),
			//2.f,
			resolution, resolution))

			break;

	case FUSE_SHADOW_MAPPING_EVSM2:

		if (g_rendererConfiguration.get_evsm2_float_precision() == 16)
		{
			g_shadowMapRTV = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			g_shadowMapRTV = DXGI_FORMAT_R32G32_FLOAT;
		}
		
		FAIL_IF(!g_shadowMapBlur.init_box_blur(
			device,
			"float2",
			g_rendererConfiguration.get_evsm2_blur_kernel_size(),
			//2.f,
			resolution, resolution))

		break;

	case FUSE_SHADOW_MAPPING_EVSM4:

		if (g_rendererConfiguration.get_evsm4_float_precision() == 16)
		{
			g_shadowMapRTV = DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		else
		{
			g_shadowMapRTV = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		FAIL_IF(!g_shadowMapBlur.init_box_blur(
			device,
			"float4",
			g_rendererConfiguration.get_evsm4_blur_kernel_size(),
			//2.f,
			resolution, resolution))

			break;

	}

	shadow_mapper_configuration shadowMapperCFG;

	shadowMapperCFG.cullMode  = D3D12_CULL_MODE_NONE;
	shadowMapperCFG.rtvFormat = g_shadowMapRTV;
	shadowMapperCFG.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	shadowMapperCFG.algorithm = algorithm;

	g_shadowMapper.shutdown();
	g_shadowMapper.init(device, shadowMapperCFG);

	g_shadowMapper.set_viewport(make_fullscreen_viewport(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution()));
	g_shadowMapper.set_scissor_rect(make_fullscreen_scissor_rect(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution()));

	/*if (createBuffers)
	{

		for (int i = 0; i < NUM_BUFFERS; i++)
		{

			auto & r = g_shadowmap32F[i];

			r.clear();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

			dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

			srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels       = -1;

			FAIL_IF (
				!r.create(
					device,
					&CD3DX12_RESOURCE_DESC::Tex2D(
						DXGI_FORMAT_R32_TYPELESS,
						resolution, resolution,
						1, 1, 1, 0,
						D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.f, 0)
				) ||
				!r.create_depth_stencil_view(device, &dsvDesc)
			)

			r->SetName(L"shadow_map_depth_buffer_32f_0");

		}

		for (int i = 0; i < NUM_BUFFERS; i++)
		{

			auto & r = g_shadowmap0[i];

			r.clear();

			FAIL_IF (
				!r.create(
					device,
					&CD3DX12_RESOURCE_DESC::Tex2D(
						rtvFormat,
						resolution, resolution,
						1, 0, 1, 0,
						D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					&CD3DX12_CLEAR_VALUE(rtvFormat, reinterpret_cast<const float*>(&XMFLOAT4(0, 0, 0, 0)))
				) ||
				!r.create_render_target_view(device) ||
				!r.create_shader_resource_view(device) ||
				!r.create_unordered_access_view(device)
			)

			r->SetName(L"shadow_map_0");

		}

		for (int i = 0; i < NUM_BUFFERS; i++)
		{

			auto & r = g_shadowmap1[i];

			r.clear();

			FAIL_IF (
				!r.create(
					device,
					&CD3DX12_RESOURCE_DESC::Tex2D(
						rtvFormat,
						resolution, resolution,
						1, 1, 1, 0,
						D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					nullptr
				) ||
				!r.create_shader_resource_view(device) ||
				!r.create_unordered_access_view(device)
			)

			r->SetName(L"shadow_map_1");

		}

	}*/

	return true;

}

render_resource_ptr renderer_application::get_shadow_map_render_target(void)
{
	return g_renderResourceManager.get_texture_2d(
		get_device(),
		get_buffer_index(),
		g_shadowMapRTV,
		g_rendererConfiguration.get_shadow_map_resolution(),
		g_rendererConfiguration.get_shadow_map_resolution(),
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		&CD3DX12_CLEAR_VALUE(g_shadowMapRTV, &std::array<float, 4>()[0]));
}