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
#include "deferred_renderer.hpp"
#include "scene.hpp"
#include "shadow_mapper.hpp"
#include "tonemapper.hpp"
#include "editor_gui.hpp"
#include "render_variables.hpp"
#include "blur.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define FAIL_IF(Condition) { if (Condition) return false; }

#define MAX_DSV 8
#define MAX_RTV 1024
#define MAX_SRV 1024

#define NUM_BUFFERS      (2u)
#define UPLOAD_HEAP_SIZE (16 << 20)

std::array<com_ptr<ID3D12CommandAllocator>, NUM_BUFFERS> g_commandAllocator;
std::array<gpu_graphics_command_list,       NUM_BUFFERS> g_commandList;
std::array<com_ptr<ID3D12CommandAllocator>, NUM_BUFFERS> g_auxCommandAllocator;
std::array<gpu_graphics_command_list,       NUM_BUFFERS> g_auxCommandList;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_cbPerFrame;

gpu_ring_buffer    g_uploadRingBuffer;
gpu_upload_manager g_uploadManager;

std::array<UINT, NUM_BUFFERS> g_gbufferSRVTable;

std::array<render_resource, NUM_BUFFERS> g_fullresRGBA16F0;
std::array<render_resource, NUM_BUFFERS> g_fullresRGBA16F1;
std::array<render_resource, NUM_BUFFERS> g_fullresRGBA16F2;
std::array<render_resource, NUM_BUFFERS> g_fullresRGBA8U0;
std::array<render_resource, NUM_BUFFERS> g_fullresRGBA8U1;

std::array<render_resource, NUM_BUFFERS> g_fullresGUI;

std::array<render_resource, NUM_BUFFERS> g_depthBuffer;

std::array<render_resource, NUM_BUFFERS> g_shadowmap32F;
std::array<render_resource, NUM_BUFFERS> g_shadowmap0;
std::array<render_resource, NUM_BUFFERS> g_shadowmap1;

std::unique_ptr<mipmap_generator> g_mipmapGenerator;
std::unique_ptr<assimp_loader>    g_sceneLoader;
								       
resource_factory                     g_resourceFactory;
std::unique_ptr<image_manager>       g_imageManager;
std::unique_ptr<mesh_manager>        g_meshManager;
std::unique_ptr<material_manager>    g_materialManager;
std::unique_ptr<gpu_mesh_manager>    g_gpuMeshManager;
std::unique_ptr<texture_manager>     g_textureManager;
std::unique_ptr<bitmap_font_manager> g_bitmapFontManager;

fps_camera_controller g_cameraController;

rocket_interface   g_rocketInterface;

bitmap_font_ptr    g_font;

scene              g_scene;
deferred_renderer  g_deferredRenderer;
tonemapper         g_tonemapper;
text_renderer      g_textRenderer;
shadow_mapper      g_shadowMapper;

renderer_configuration g_rendererConfiguration;

blur           g_shadowMapBlur;
alpha_composer g_alphaComposer;

#ifdef FUSE_USE_EDITOR_GUI

editor_gui g_editorGUI;
bool       g_showGUI = true;

#endif

bool renderer_application::on_device_created(ID3D12Device * device, gpu_command_queue & commandQueue)
{

	g_mipmapGenerator = std::make_unique<mipmap_generator>(device);

	if (!g_uploadRingBuffer.create(device, UPLOAD_HEAP_SIZE) ||
		!g_uploadManager.init(device, NUM_BUFFERS))
	{
		return false;
	}

	/* Resource managers */

	g_imageManager      = std::make_unique<image_manager>();
	g_materialManager   = std::make_unique<material_manager>();
	g_meshManager       = std::make_unique<mesh_manager>();
	g_bitmapFontManager = std::make_unique<bitmap_font_manager>();
	g_gpuMeshManager    = std::make_unique<gpu_mesh_manager>(device, commandQueue, &g_uploadManager, &g_uploadRingBuffer);
	g_textureManager    = std::make_unique<texture_manager>(device, commandQueue, &g_uploadManager, &g_uploadRingBuffer);

	g_resourceFactory.register_manager(g_imageManager.get());
	g_resourceFactory.register_manager(g_materialManager.get());
	g_resourceFactory.register_manager(g_bitmapFontManager.get());
	g_resourceFactory.register_manager(g_meshManager.get());
	g_resourceFactory.register_manager(g_gpuMeshManager.get());
	g_resourceFactory.register_manager(g_textureManager.get());

	/* Load scene */

	//g_sceneLoader = std::make_unique<assimp_loader>("scene/default.fbx");
	g_sceneLoader = std::make_unique<assimp_loader>("scene/cornellbox_solid.fbx");

	//g_sceneLoader = std::make_unique<assimp_loader>("scene/sponza.fbx", ppsteps);

	FAIL_IF (
	    !g_scene.import_static_objects(g_sceneLoader.get()) ||
	    !g_scene.import_cameras(g_sceneLoader.get()) ||
	    !g_scene.import_lights(g_sceneLoader.get())
	)

	auto light = *g_scene.get_lights().first;

	light->direction = XMFLOAT3(-0.281581f, 0.522937f, -0.804518f);
	light->intensity *= .2f;
	//g_scene.get_active_camera()->set_zfar(10000.f);

	//g_cameraController = std::make_unique<fps_camera_controller>(g_scene.get_active_camera());

	g_cameraController.set_camera(g_scene.get_active_camera());
	g_cameraController.set_speed(XMFLOAT3(200, 200, 200));

	g_scene.get_active_camera()->set_orientation(XMQuaternionIdentity());

	/* Prepare the command list */

	for (unsigned int bufferIndex = 0; bufferIndex < NUM_BUFFERS; bufferIndex++)
	{
		
		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator[bufferIndex])));
		g_commandList[bufferIndex].init(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[bufferIndex].get(), nullptr);

		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_auxCommandAllocator[bufferIndex])));
		g_auxCommandList[bufferIndex].init(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_auxCommandAllocator[bufferIndex].get(), nullptr);

		FUSE_HR_CHECK(g_commandAllocator[bufferIndex]->SetName(L"main_command_allocator"));
		FUSE_HR_CHECK(g_commandList[bufferIndex]->SetName(L"main_command_list"));

		FUSE_HR_CHECK(g_auxCommandAllocator[bufferIndex]->SetName(L"aux_command_allocator"));
		FUSE_HR_CHECK(g_auxCommandList[bufferIndex]->SetName(L"aux_command_list"));

		FUSE_HR_CHECK(g_commandList[bufferIndex]->Close());
		FUSE_HR_CHECK(g_auxCommandList[bufferIndex]->Close());

	}

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

	}

	g_font = resource_factory::get_singleton_pointer()->create<bitmap_font>("bitmap_font", "fonts/consolas_regular_12");

	/* Initialize renderer */

	deferred_renderer_configuration rendererCFG;

	rendererCFG.gbufferFormat[0]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[1]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[2]       = DXGI_FORMAT_R8G8B8A8_UNORM;
	rendererCFG.gbufferFormat[3]       = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.dsvFormat              = DXGI_FORMAT_D24_UNORM_S8_UINT;
	rendererCFG.shadingFormat          = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.shadowMappingAlgorithm = g_rendererConfiguration.get_shadow_mapping_algorithm();

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

	rocketCFG.rtvFormat   = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	FAIL_IF (
	    !g_rocketInterface.init(device, commandQueue, &g_uploadManager, &g_uploadRingBuffer, rocketCFG) ||
		!g_editorGUI.init(&g_scene, &g_rendererConfiguration)
	)

	g_editorGUI.set_render_options_visibility(true);
	g_editorGUI.set_light_panel_visibility(true);

#endif

	alpha_composer_configuration composerCFG;

	composerCFG.blendDesc                              = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	composerCFG.blendDesc.RenderTarget[0].BlendEnable  = TRUE;
	composerCFG.blendDesc.RenderTarget[0].SrcBlend     = D3D12_BLEND_SRC_ALPHA;
	composerCFG.blendDesc.RenderTarget[0].DestBlend    = D3D12_BLEND_INV_SRC_ALPHA;
	composerCFG.blendDesc.RenderTarget[0].BlendOp      = D3D12_BLEND_OP_ADD;
	composerCFG.blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_MAX;

	composerCFG.rtvFormat                             = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (g_deferredRenderer.init(device, rendererCFG) &&
	    g_tonemapper.init(device) &&
	    g_textRenderer.init(device) &&
	    g_alphaComposer.init(device, composerCFG) &&
	    create_shadow_map_resources(device))
	{

		/* Wait for the initialization operations to be done on the GPU
		   before starting the rendering and resetting the command allocator */

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

void renderer_application::on_device_released(ID3D12Device * device)
{

	std::for_each(g_commandAllocator.begin(), g_commandAllocator.end(), [](auto & r) { r.reset(); });
	std::for_each(g_commandList.begin(), g_commandList.end(), [](auto & r) { r.reset(); });

	std::for_each(g_fullresRGBA16F0.begin(), g_fullresRGBA16F0.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA16F1.begin(), g_fullresRGBA16F1.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA16F2.begin(), g_fullresRGBA16F2.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA8U0.begin(), g_fullresRGBA8U0.end(), [](auto & r) { r.reset(); });
	std::for_each(g_depthBuffer.begin(), g_depthBuffer.end(), [](auto & r) { r.reset(); });

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

	D3D12_VIEWPORT viewport = {};

	viewport.Width    = desc->Width;
	viewport.Height   = desc->Height;
	viewport.MaxDepth = 1.f;

	D3D12_RECT scissorRect = {};

	scissorRect.right  = desc->Width;
	scissorRect.bottom = desc->Height;

	g_deferredRenderer.set_viewport(viewport);
	g_deferredRenderer.set_scissor_rect(scissorRect);

	g_textRenderer.set_viewport(viewport);
	g_textRenderer.set_scissor_rect(scissorRect);

	g_alphaComposer.set_viewport(viewport);
	g_alphaComposer.set_scissor_rect(scissorRect);

	g_cameraController.on_resize(desc->Width, desc->Height);

	camera * mainCamera = g_scene.get_active_camera();

	if (mainCamera)
	{
		mainCamera->set_aspect_ratio((float) desc->Width / desc->Height);
	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_depthBuffer[i];

		r.clear();

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_D24_UNORM_S8_UINT,
					desc->Width, desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0)
			)  ||
			!r.create_depth_stencil_view(device)
		)

		r->SetName(L"main_depth_buffer");

	}

	const float black[4] = { 0 };

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F0[i];
		
		r.clear();

		// gbuffer 0
		r.set_srv_token(g_gbufferSRVTable[i]);

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
			)  ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device)
		)

		r->SetName(L"fullres_rgba16f0");
		
	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F1[i];

		r.clear();

		// gbuffer 1
		r.set_srv_token(g_gbufferSRVTable[i] + 1);

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
			)  ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device)
		)

		r->SetName(L"fullres_rgba16f1");

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F2[i];

		r.clear();

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
			) ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device)
		)

		r->SetName(L"fullres_rgba16f2");

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U0[i];

		r.clear();

		// gbuffer 2
		r.set_srv_token(g_gbufferSRVTable[i] + 2);

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R8G8B8A8_UNORM,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, black)
			) ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device) ||
			!r.create_unordered_access_view(device)
		)

		r->SetName(L"fullres_rgba8u0");

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U1[i];

		r.clear();

		// gbuffer 3
		r.set_srv_token(g_gbufferSRVTable[i] + 3);

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black)
			) ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device)
		)

		r->SetName(L"fullres_rgba8u1");

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresGUI[i];

		r.clear();

		FAIL_IF (
			!r.create(
				device,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, black)
			) ||
			!r.create_render_target_view(device) ||
			!r.create_shader_resource_view(device)
		)

			r->SetName(L"fullres_gui");

	}

#ifdef FUSE_USE_EDITOR_GUI
	g_rocketInterface.on_resize(desc->Width, desc->Height);
	g_editorGUI.on_resize(desc->Width, desc->Height);
#endif

	return true;

}

LRESULT CALLBACK renderer_application::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
{

#define __KEY_DOWN(Key) (wParam == Key && ((KF_UP << 16) & lParam) == 0)

	if (!code)
	{

		if (__KEY_DOWN(VK_F11))
		{
			bool goFullscreen = !is_fullscreen();
			set_fullscreen(goFullscreen);
			set_cursor(goFullscreen, goFullscreen);
			g_cameraController.set_auto_center_mouse(goFullscreen);
		}
		else if (__KEY_DOWN(VK_F9))
		{
			g_editorGUI.set_debugger_visibility(!g_editorGUI.get_debugger_visibility());
		}
		else if (__KEY_DOWN(VK_F4))
		{
			g_showGUI = !g_showGUI;
		}
		else
		{
			(g_showGUI && g_editorGUI.on_keyboard(wParam, lParam)) ||
				g_cameraController.on_keyboard(wParam, lParam);
		}

	}

	return base_type::on_keyboard(code, wParam, lParam);

}

LRESULT CALLBACK renderer_application::on_mouse(int code, WPARAM wParam, LPARAM lParam)
{

	if (!code)
	{
		(g_showGUI && g_editorGUI.on_mouse(wParam, lParam)) ||
			g_cameraController.on_mouse(wParam, lParam);
	}

	return base_type::on_mouse(code, wParam, lParam);

}

LRESULT renderer_application::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_editorGUI.on_message(hWnd, uMsg, wParam, lParam);
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

void renderer_application::upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue, ID3D12Resource * cbPerFrameBuffer)
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

	cbPerFrame.screen.resolution        = XMUINT2(get_screen_width(), get_screen_height());
	cbPerFrame.screen.orthoProjection   = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0, get_screen_width(), get_screen_height(), 0, 0, 1));

	/* Render variables */

	cbPerFrame.rvars.shadowMapResolution = g_rendererConfiguration.get_shadow_map_resolution();
	cbPerFrame.rvars.vsmMinVariance      = g_rendererConfiguration.get_vsm_min_variance();
	cbPerFrame.rvars.vsmMinBleeding      = g_rendererConfiguration.get_vsm_min_bleeding();

	cbPerFrame.rvars.evsm2MinVariance    = g_rendererConfiguration.get_evsm2_min_variance();
	cbPerFrame.rvars.evsm2MinBleeding    = g_rendererConfiguration.get_evsm2_min_bleeding();
	cbPerFrame.rvars.evsm2Exponent       = esm_clamp_exponent(g_rendererConfiguration.get_evsm2_exponent(), g_rendererConfiguration.get_evsm2_float_precision());

	cbPerFrame.rvars.evsm4MinVariance = g_rendererConfiguration.get_evsm4_min_variance();
	cbPerFrame.rvars.evsm4MinBleeding = g_rendererConfiguration.get_evsm4_min_bleeding();
	cbPerFrame.rvars.evsm4PosExponent = esm_clamp_exponent_positive(g_rendererConfiguration.get_evsm4_positive_exponent(), g_rendererConfiguration.get_evsm4_float_precision());
	cbPerFrame.rvars.evsm4NegExponent = esm_clamp_exponent_positive(g_rendererConfiguration.get_evsm4_negative_exponent(), g_rendererConfiguration.get_evsm4_float_precision());

	/* Upload */

	D3D12_GPU_VIRTUAL_ADDRESS address;
	D3D12_GPU_VIRTUAL_ADDRESS heapAddress = g_uploadRingBuffer.get_heap()->GetGPUVirtualAddress();

	void * cbData = g_uploadRingBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_frame), &address);

	memcpy(cbData, &cbPerFrame, sizeof(cb_per_frame));

	g_uploadManager.upload_buffer(commandQueue, cbPerFrameBuffer, 0, g_uploadRingBuffer.get_heap(), address - heapAddress, sizeof(cb_per_frame));

}

void renderer_application::on_render(ID3D12Device * device, gpu_command_queue & commandQueue, const render_resource & backBuffer, UINT bufferIndex)
{

	update_renderer_configuration(device, commandQueue);

	/* Reset command list and command allocator */

	gpu_graphics_command_list & commandList    = g_commandList[bufferIndex];
	gpu_graphics_command_list & auxCommandList = g_auxCommandList[bufferIndex];

	g_uploadManager.set_command_list_index(bufferIndex);
	commandQueue.set_aux_command_list(auxCommandList);

	auxCommandList.reset_command_allocator();
	commandList.reset_command_allocator();

	/* Setup render */

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	upload_per_frame_resources(device, commandQueue, g_cbPerFrame[bufferIndex].get());

	/* Clear the buffers */

	render_resource * gbuffer[] = {
		std::addressof(g_fullresRGBA16F0[bufferIndex]),
		std::addressof(g_fullresRGBA16F1[bufferIndex]),
		std::addressof(g_fullresRGBA8U0[bufferIndex]),
		std::addressof(g_fullresRGBA8U1[bufferIndex]),
		std::addressof(g_depthBuffer[bufferIndex])
	};

	auto dsvHandle = g_depthBuffer[bufferIndex].get_dsv_cpu_descriptor_handle();

	float clearColor[4] = { 0.f };

	commandList.reset_command_list(nullptr);

	commandList->ClearRenderTargetView(gbuffer[0]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[1]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[2]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[3]->get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	commandList->ClearRenderTargetView(g_fullresGUI[bufferIndex].get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(g_depthBuffer[bufferIndex].get_dsv_cpu_descriptor_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	commandList.resource_barrier_transition(g_cbPerFrame[bufferIndex].get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

	/* Draw GUI */

	draw_gui(device, commandQueue, commandList);

	/* GBuffer rendering */

	auto staticObjects = g_scene.get_static_objects();

	g_deferredRenderer.render_gbuffer(
		device,
		commandQueue,
		commandList,
		&g_uploadRingBuffer,
		cbPerFrameAddress,
		gbuffer,
		g_depthBuffer[bufferIndex],
		g_scene.get_active_camera(),
		staticObjects.first,
		staticObjects.second);

	/* Prepare for shading */

	commandList.reset_command_list(nullptr);

	commandList.resource_barrier_transition(g_fullresRGBA16F2[bufferIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(g_fullresRGBA16F2[bufferIndex].get_rtv_cpu_descriptor_handle(), clearColor, 0, nullptr);

	FUSE_HR_CHECK(commandList->Close());

	commandQueue.execute(commandList);

	/* Shading */

	auto lightsIterators = g_scene.get_lights();

	for (auto it = lightsIterators.first; it != lightsIterators.second; it++)
	{

		light * currentLight = *it;

		shadow_map_info shadowMapInfo, * pShadowMapInfo = nullptr;

		if (currentLight->type == FUSE_LIGHT_TYPE_DIRECTIONAL)
		{

			XMVECTOR direction = to_vector(currentLight->direction);

			XMMATRIX viewMatrix = XMMatrixLookAtLH(
				XMVectorZero(),
				direction,
				XMVectorSet(currentLight->direction.y > .99f ? 1.f : 0.f, currentLight->direction.y > .99f ? 0.f : 1.f, 0.f, 0.f));

			aabb currentAABB = aabb::from_min_max(
				XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX),
				XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));

			for (auto it = staticObjects.first; it != staticObjects.second; it++)
			{
				XMMATRIX worldView       = XMMatrixMultiply((*it)->get_world_matrix(), viewMatrix);
				sphere transformedSphere = transform_affine((*it)->get_bounding_sphere(), worldView);
				aabb objectAABB          = bounding_aabb(transformedSphere);
				currentAABB              = currentAABB + objectAABB;
			}

			auto aabbMin = to_float3(currentAABB.get_min());
			auto aabbMax = to_float3(currentAABB.get_max());

			XMMATRIX orthoMatrix = XMMatrixOrthographicOffCenterLH(aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMax.z, aabbMin.z);

			shadowMapInfo.shadowMap   = std::addressof(g_shadowmap0[bufferIndex]);
			shadowMapInfo.lightMatrix = XMMatrixMultiply(viewMatrix, orthoMatrix);

			g_shadowMapper.render(
				device,
				commandQueue,
				commandList,
				&g_uploadRingBuffer,
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
			&g_uploadRingBuffer,
			cbPerFrameAddress,
			g_fullresRGBA16F2[bufferIndex],
			gbuffer,
			currentLight,
			pShadowMapInfo);

	}

	/* Tonemap */

	g_tonemapper.render(
		device,
		commandQueue,
		commandList,
		g_fullresRGBA16F2[bufferIndex],
		g_fullresRGBA8U0[bufferIndex],
		get_screen_width(),
		get_screen_height());

	/* Finally copy to backbuffer */

	commandList.reset_command_list(nullptr);

	commandList.resource_barrier_transition(g_fullresRGBA8U0[bufferIndex].get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList.resource_barrier_transition(backBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(backBuffer.get(), g_fullresRGBA8U0[bufferIndex].get());

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);	

	/* Compose */

	render_resource * composeResources[] = {
		std::addressof(g_fullresGUI[bufferIndex])
	};

	g_alphaComposer.render(
		commandQueue,
		commandList,
		backBuffer,
		composeResources,
		_countof(composeResources));

	/* Present */

	commandList.reset_command_list(nullptr);

	commandList.resource_barrier_transition(backBuffer.get(), D3D12_RESOURCE_STATE_PRESENT);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

}

void renderer_application::draw_gui(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList)
{

	uint32_t bufferIndex = get_buffer_index();

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	auto guiRTV = g_fullresGUI[bufferIndex].get_rtv_cpu_descriptor_handle();

	std::stringstream guiSS;

	guiSS << "FPS: " << get_fps() << std::endl;

	g_textRenderer.render(
		device,
		commandQueue,
		commandList,
		g_fullresGUI[bufferIndex].get(),
		&guiRTV,
		cbPerFrameAddress,
		&g_uploadRingBuffer,
		g_font.get(),
		guiSS.str().c_str(),
		XMFLOAT2(16, 16),
		XMFLOAT4(1, 1, 0, .5f));

#ifdef FUSE_USE_EDITOR_GUI

	if (g_showGUI)
	{

		g_rocketInterface.render_begin(
			commandList,
			cbPerFrameAddress,
			g_fullresGUI[bufferIndex].get(),
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
}

bool renderer_application::create_shadow_map_resources(ID3D12Device * device, bool createBuffers)
{

	uint32_t resolution = g_rendererConfiguration.get_shadow_map_resolution();
	shadow_mapping_algorithm algorithm = g_rendererConfiguration.get_shadow_mapping_algorithm();

	DXGI_FORMAT rtvFormat;

	switch (algorithm)
	{

	case FUSE_SHADOW_MAPPING_NONE:
		return true;

	case FUSE_SHADOW_MAPPING_VSM:

		if (g_rendererConfiguration.get_evsm2_float_precision() == 16)
		{
			rtvFormat = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			rtvFormat = DXGI_FORMAT_R32G32_FLOAT;
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
			rtvFormat = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			rtvFormat = DXGI_FORMAT_R32G32_FLOAT;
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
			rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		else
		{
			rtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
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
	shadowMapperCFG.rtvFormat = rtvFormat;
	shadowMapperCFG.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	shadowMapperCFG.algorithm = algorithm;

	g_shadowMapper.shutdown();
	g_shadowMapper.init(device, shadowMapperCFG);

	g_shadowMapper.set_viewport(make_fullscreen_viewport(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution()));
	g_shadowMapper.set_scissor_rect(make_fullscreen_scissor_rect(g_rendererConfiguration.get_shadow_map_resolution(), g_rendererConfiguration.get_shadow_map_resolution()));

	if (createBuffers)
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

			FAIL_IF(
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

	}

	return true;

}