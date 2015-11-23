#include "renderer_application.hpp"

#include <fuse/logger.hpp>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <fuse/gpu_upload_manager.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_global_resource_state.hpp>

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

#include <fuse/rocket_interface.hpp>

#include <Rocket/Controls.h>

#include "cbuffer_structs.hpp"
#include "deferred_renderer.hpp"
#include "scene.hpp"
#include "shadow_mapper.hpp"
#include "tonemapper.hpp"
#include "editor_gui.hpp"
#include "render_variables.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define NUM_BUFFERS      (2u)
#define NUM_RTVS         (6u)
#define NUM_SRVS_UAVS    (8u)
#define NUM_DSVS         (2u)
#define UPLOAD_HEAP_SIZE (16 << 20)

#define RTV_HANDLE(FrameIndex, Index)     ((NUM_RTVS * FrameIndex) + Index)
#define SRV_UAV_HANDLE(FrameIndex, Index) ((NUM_SRVS_UAVS * FrameIndex) + Index)
#define DSV_HANDLE(FrameIndex, Index)     ((NUM_DSVS * FrameIndex) + Index)

#define RTV_RGBA16F0(FrameIndex)  RTV_HANDLE(FrameIndex, 0)
#define RTV_RGBA16F1(FrameIndex)  RTV_HANDLE(FrameIndex, 1)
#define RTV_RGBA8U0(FrameIndex)   RTV_HANDLE(FrameIndex, 2)
#define RTV_RGBA16F2(FrameIndex)  RTV_HANDLE(FrameIndex, 3)
#define RTV_RGBA16F3(FrameIndex)  RTV_HANDLE(FrameIndex, 4)
#define RTV_RG16F0(FrameIndex)    RTV_HANDLE(FrameIndex, 5) 

#define SRV_RGBA16F0(FrameIndex)  SRV_UAV_HANDLE(FrameIndex, 0)
#define SRV_RGBA16F1(FrameIndex)  SRV_UAV_HANDLE(FrameIndex, 1)
#define SRV_RGBA8U0(FrameIndex)   SRV_UAV_HANDLE(FrameIndex, 2)
#define SRV_RGBA16F3(FrameIndex)  SRV_UAV_HANDLE(FrameIndex, 3)
#define SRV_RGBA16F2(FrameIndex)  SRV_UAV_HANDLE(FrameIndex, 4)
#define SRV_SHADOWMAP(FrameIndex) SRV_UAV_HANDLE(FrameIndex, 5)
#define SRV_RG16F0(FrameIndex)    SRV_UAV_HANDLE(FrameIndex, 6)
#define SRV_GBUFFER(FrameIndex)   SRV_RGBA16F0(FrameIndex)

#define UAV_RGBA8U0(FrameIndex)   SRV_UAV_HANDLE(FrameIndex, 7)

#define DSV_DEPTH(FrameIndex)     DSV_HANDLE(FrameIndex, 0)
#define DSV_SHADOWMAP(FrameIndex) DSV_HANDLE(FrameIndex, 1)

#define SHADOW_MAP_RESOLUTION 1024

std::array<com_ptr<ID3D12CommandAllocator>, NUM_BUFFERS> g_commandAllocator;
std::array<gpu_graphics_command_list,       NUM_BUFFERS> g_commandList;
std::array<com_ptr<ID3D12CommandAllocator>, NUM_BUFFERS> g_auxCommandAllocator;
std::array<gpu_graphics_command_list,       NUM_BUFFERS> g_auxCommandList;
								       
com_ptr<ID3D12DescriptorHeap> g_dsvHeap;
com_ptr<ID3D12DescriptorHeap> g_rtvHeap;
com_ptr<ID3D12DescriptorHeap> g_srvHeap;

D3D12_CPU_DESCRIPTOR_HANDLE   g_dsvHandle;
D3D12_CPU_DESCRIPTOR_HANDLE   g_rtvHandle;
D3D12_GPU_DESCRIPTOR_HANDLE   g_srvHandle;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_cbPerFrame;

gpu_ring_buffer    g_uploadRingBuffer;
gpu_upload_manager g_uploadManager;
								   	   
//com_ptr<ID3D12Resource>                g_fullresRGBA16F[2];
//com_ptr<ID3D12Resource>                g_fullresRGBA8U;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F0;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F1;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F2;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA8U0;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA8U1;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_depthBuffer;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_shadowmap32F;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_shadowmapRG16F;

std::unique_ptr<mipmap_generator> g_mipmapGenerator;
std::unique_ptr<assimp_loader>    g_sceneLoader;
								       
resource_factory                     g_resourceFactory;
std::unique_ptr<image_manager>       g_imageManager;
std::unique_ptr<mesh_manager>        g_meshManager;
std::unique_ptr<material_manager>    g_materialManager;
std::unique_ptr<gpu_mesh_manager>    g_gpuMeshManager;
std::unique_ptr<texture_manager>     g_textureManager;
std::unique_ptr<bitmap_font_manager> g_bitmapFontManager;

std::unique_ptr<fps_camera_controller> g_cameraController;

rocket_interface   g_rocketInterface;

bitmap_font_ptr    g_font;

scene              g_scene;
deferred_renderer  g_deferredRenderer;
tonemapper         g_tonemapper;
text_renderer      g_textRenderer;
shadow_mapper      g_shadowMapper;

renderer_configuration g_rendererConfiguration;

#ifdef FUSE_USE_EDITOR_GUI

editor_gui g_editorGUI;
bool       g_showGUI = true;

#endif

gpu_global_resource_state g_globalResourceState;

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

	if (!g_scene.import_static_objects(g_sceneLoader.get()) ||
		!g_scene.import_cameras(g_sceneLoader.get()) ||
		!g_scene.import_lights(g_sceneLoader.get()))
	{
		return false;
	}

	auto light = *g_scene.get_lights().first;

	light->direction = XMFLOAT3(-0.281581f, 0.522937f, -0.804518f);
	light->intensity *= .2f;
	//g_scene.get_active_camera()->set_zfar(10000.f);

	g_cameraController = std::make_unique<fps_camera_controller>(g_scene.get_active_camera());
	g_cameraController->set_speed(XMFLOAT3(200, 200, 200));

	g_scene.get_active_camera()->set_orientation(XMQuaternionIdentity());

	/* Prepare the command list */

	for (unsigned int bufferIndex = 0; bufferIndex < NUM_BUFFERS; bufferIndex++)
	{
		
		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator[bufferIndex])));
		FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[bufferIndex].get(), nullptr, IID_PPV_ARGS(g_commandList[bufferIndex].get_address())));

		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_auxCommandAllocator[bufferIndex])));
		FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_auxCommandAllocator[bufferIndex].get(), nullptr, IID_PPV_ARGS(g_auxCommandList[bufferIndex].get_address())));

		FUSE_HR_CHECK(g_commandAllocator[bufferIndex]->SetName(L"main_command_allocator"));
		FUSE_HR_CHECK(g_commandList[bufferIndex]->SetName(L"main_command_list"));

		FUSE_HR_CHECK(g_auxCommandAllocator[bufferIndex]->SetName(L"aux_command_allocator"));
		FUSE_HR_CHECK(g_auxCommandList[bufferIndex]->SetName(L"aux_command_list"));

		FUSE_HR_CHECK(g_commandList[bufferIndex]->Close());
		FUSE_HR_CHECK(g_auxCommandList[bufferIndex]->Close());

	}	

	/* Create descriptor heaps */

	D3D12_DESCRIPTOR_HEAP_DESC gbufferRTVDesc = { };

	gbufferRTVDesc.NumDescriptors = NUM_RTVS * NUM_BUFFERS;
	gbufferRTVDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	gbufferRTVDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	D3D12_DESCRIPTOR_HEAP_DESC gbufferDSVDesc = {};

	gbufferDSVDesc.NumDescriptors = NUM_DSVS * NUM_BUFFERS;
	gbufferDSVDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	gbufferDSVDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};

	srvDesc.NumDescriptors = NUM_SRVS_UAVS * NUM_BUFFERS;
	srvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FUSE_HR_FAILED(device->CreateDescriptorHeap(&gbufferRTVDesc, IID_PPV_ARGS(&g_rtvHeap))) ||
	    FUSE_HR_FAILED(device->CreateDescriptorHeap(&gbufferDSVDesc, IID_PPV_ARGS(&g_dsvHeap))) ||
	    FUSE_HR_FAILED(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap))))
	{
		return false;
	}

	g_dsvHandle = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	g_rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	g_srvHandle = g_srvHeap->GetGPUDescriptorHandleForHeapStart();

	/* Create constant buffers */

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cb_per_frame)),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			nullptr,
			IID_PPV_ARGS(&g_cbPerFrame[i])))
		{
			return false;
		}

	}

	g_font = resource_factory::get_singleton_pointer()->create<bitmap_font>("bitmap_font", "fonts/consolas_regular_12");

	/* Initialize renderer */

	deferred_renderer_configuration rendererCFG;

	rendererCFG.gbufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rendererCFG.gbufferFormat[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.dsvFormat        = DXGI_FORMAT_D24_UNORM_S8_UINT;
	rendererCFG.shadingFormat    = DXGI_FORMAT_R16G16B16A16_FLOAT;

	shadow_mapper_configuration shadowMapperCFG;

	shadowMapperCFG.cullMode    = D3D12_CULL_MODE_NONE;
	shadowMapperCFG.evsm2Format = DXGI_FORMAT_R16G16_FLOAT;
	shadowMapperCFG.dsvFormat   = DXGI_FORMAT_D32_FLOAT;

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

	rocketCFG.blendDesc                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	rocketCFG.blendDesc.RenderTarget[0].BlendEnable = TRUE;
	rocketCFG.blendDesc.RenderTarget[0].SrcBlend    = D3D12_BLEND_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].DestBlend   = D3D12_BLEND_INV_SRC_ALPHA;
	rocketCFG.blendDesc.RenderTarget[0].BlendOp     = D3D12_BLEND_OP_ADD;

	rocketCFG.rtvFormat                             = DXGI_FORMAT_R8G8B8A8_UNORM;
	rocketCFG.maxTextures                           = 16;

	if (!g_rocketInterface.init(device, commandQueue, &g_uploadManager, &g_uploadRingBuffer, rocketCFG) ||
		!g_editorGUI.init(&g_scene, &g_rendererConfiguration))
	{
		return false;
	}

	g_editorGUI.set_render_options_visibility(true);
	g_editorGUI.set_light_panel_visibility(true);

#endif

	return g_deferredRenderer.init(device, rendererCFG) &&
	       g_tonemapper.init(device) &&
	       g_textRenderer.init(device) &&
	       g_shadowMapper.init(device, shadowMapperCFG);

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

	g_rtvHeap.reset();
	g_dsvHeap.reset();

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

	g_cameraController->on_resize(desc->Width, desc->Height);

	camera * mainCamera = g_scene.get_active_camera();

	if (mainCamera)
	{
		mainCamera->set_aspect_ratio((float) desc->Width / desc->Height);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = g_srvHeap->GetCPUDescriptorHandleForHeapStart();

	if (!create_shadow_map_buffers(device))
	{
		return false;
	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_depthBuffer[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_D24_UNORM_S8_UINT,
					desc->Width, desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0),
				IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"main_depth_buffer");

		device->CreateDepthStencilView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, DSV_DEPTH(i), get_dsv_descriptor_size()));

	}

	const float black[4] = { 0 };

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F0[i];
		
		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
				IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f0");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		
		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F0(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RGBA16F0(i), get_srv_descriptor_size()));
		
	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F1[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
				IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f1");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F1(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RGBA16F1(i), get_srv_descriptor_size()));


	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F2[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R16G16B16A16_FLOAT,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
				IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f2");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F2(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RGBA16F2(i), get_srv_descriptor_size()));


	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U0[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R8G8B8A8_UNORM,
					desc->Width,
					desc->Height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				//D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, black),
				IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"fullres_rgba8u0");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA8U0(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RGBA8U0(i), get_srv_descriptor_size()));

		device->CreateUnorderedAccessView(r.get(),
			nullptr, nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, UAV_RGBA8U0(i), get_uav_descriptor_size()));

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U1[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				desc->Width,
				desc->Height,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
			IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f3");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F3(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RGBA16F3(i), get_srv_descriptor_size()));

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
			g_cameraController->set_auto_center_mouse(goFullscreen);
		}
		/*else if (__KEY_DOWN(VK_F5))
		{
			g_editorGUI.select_object(*g_scene.get_static_objects().first);
		}*/
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
				g_cameraController->on_keyboard(wParam, lParam);
		}

	}

	return base_type::on_keyboard(code, wParam, lParam);

}

LRESULT CALLBACK renderer_application::on_mouse(int code, WPARAM wParam, LPARAM lParam)
{

	if (!code)
	{
		(g_showGUI && g_editorGUI.on_mouse(wParam, lParam)) ||
			g_cameraController->on_mouse(wParam, lParam);
	}

	return base_type::on_mouse(code, wParam, lParam);

}

LRESULT renderer_application::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_editorGUI.on_message(hWnd, uMsg, wParam, lParam);
}

void renderer_application::on_update(float dt)
{

	g_cameraController->on_update(dt);

#if FUSE_USE_EDITOR_GUI

	if (g_showGUI)
	{
		g_editorGUI.update();
	}

#endif

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

	cbPerFrame.camera.position          = to_float3(camera->get_position());
	cbPerFrame.camera.fovy              = camera->get_fovy();
	cbPerFrame.camera.aspectRatio       = camera->get_aspect_ratio();
	cbPerFrame.camera.znear             = camera->get_znear();
	cbPerFrame.camera.zfar              = camera->get_zfar();
	cbPerFrame.camera.view              = XMMatrixTranspose(view);
	cbPerFrame.camera.projection        = XMMatrixTranspose(projection);
	cbPerFrame.camera.viewProjection    = XMMatrixTranspose(viewProjection);
	cbPerFrame.camera.invViewProjection = XMMatrixTranspose(invViewProjection);

	cbPerFrame.screen.resolution        = XMUINT2(get_screen_width(), get_screen_height());
	cbPerFrame.screen.orthoProjection = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0, get_screen_width(), get_screen_height(), 0, 0, 1));

	cbPerFrame.rvars.vsmMinVariance = g_rendererConfiguration.get_vsm_min_variance();
	cbPerFrame.rvars.vsmMinBleeding = g_rendererConfiguration.get_vsm_min_bleeding();

	D3D12_GPU_VIRTUAL_ADDRESS address;
	D3D12_GPU_VIRTUAL_ADDRESS heapAddress = g_uploadRingBuffer.get_heap()->GetGPUVirtualAddress();

	void * cbData = g_uploadRingBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_frame), &address);

	memcpy(cbData, &cbPerFrame, sizeof(cb_per_frame));

	g_uploadManager.upload_buffer(commandQueue, cbPerFrameBuffer, 0, g_uploadRingBuffer.get_heap(), address - heapAddress, sizeof(cb_per_frame));

}

void renderer_application::on_render(ID3D12Device * device, gpu_command_queue & commandQueue, ID3D12Resource * backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor, UINT bufferIndex)
{

	/* Reset command list and command allocator */

	ID3D12CommandAllocator    * commandAllocator = g_commandAllocator[bufferIndex].get();
	gpu_graphics_command_list & commandList      = g_commandList[bufferIndex];

	ID3D12CommandAllocator    * auxCommandAllocator = g_auxCommandAllocator[bufferIndex].get();
	gpu_graphics_command_list & auxCommandList      = g_auxCommandList[bufferIndex];

	g_uploadManager.set_command_list_index(bufferIndex);
	commandQueue.set_aux_command_allocator(auxCommandAllocator);
	commandQueue.set_aux_command_list(auxCommandList.get());

	FUSE_HR_CHECK(commandAllocator->Reset());
	FUSE_HR_CHECK(auxCommandAllocator->Reset());

	/* Setup render */

	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress = g_cbPerFrame[bufferIndex]->GetGPUVirtualAddress();

	upload_per_frame_resources(device, commandQueue, g_cbPerFrame[bufferIndex].get());

	/* Clear the buffers */

	CD3DX12_CPU_DESCRIPTOR_HANDLE gbufferHandles[] = {
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F0(bufferIndex), get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F1(bufferIndex), get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA8U0(bufferIndex),  get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RGBA16F3(bufferIndex),  get_rtv_descriptor_size())
	};

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(g_dsvHandle, DSV_DEPTH(bufferIndex), get_dsv_descriptor_size());
	
	float clearColor[4] = { 0.f };

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	commandList->ClearRenderTargetView(gbufferHandles[0], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbufferHandles[1], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbufferHandles[2], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbufferHandles[3], clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	commandList.resource_barrier_transition(g_cbPerFrame[bufferIndex].get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

	/* GBuffer rendering */

	ID3D12Resource * gbuffer[] = {
		g_fullresRGBA16F0[bufferIndex].get(),
		g_fullresRGBA16F1[bufferIndex].get(),
		g_fullresRGBA8U0[bufferIndex].get(),
		g_fullresRGBA8U1[bufferIndex].get(),
		g_depthBuffer[bufferIndex].get()
	};

	auto staticObjects = g_scene.get_static_objects();

	g_deferredRenderer.render_gbuffer(
		device,
		commandQueue,
		commandAllocator,
		commandList,
		&g_uploadRingBuffer,
		cbPerFrameAddress,
		gbufferHandles,
		&dsvHandle,
		gbuffer,
		g_scene.get_active_camera(),
		staticObjects.first,
		staticObjects.second);

	/* Prepare for shading */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	CD3DX12_CPU_DESCRIPTOR_HANDLE finalRTV(g_rtvHandle, RTV_RGBA16F2(bufferIndex), get_rtv_descriptor_size());

	commandList.resource_barrier_transition(g_fullresRGBA16F2[bufferIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(finalRTV, clearColor, 0, nullptr);

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
				XMMATRIX worldView = XMMatrixMultiply((*it)->get_world_matrix(), viewMatrix);
				sphere transformedSphere = transform_affine((*it)->get_bounding_sphere(), worldView);
				aabb objectAABB = bounding_aabb(transformedSphere);
				currentAABB = currentAABB + objectAABB;
			}

			auto aabbMin = to_float3(currentAABB.get_min());
			auto aabbMax = to_float3(currentAABB.get_max());

			XMMATRIX orthoMatrix = XMMatrixOrthographicOffCenterLH(aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMax.z, aabbMin.z);

			shadowMapInfo.shadowMap      = g_shadowmapRG16F[bufferIndex].get();
			shadowMapInfo.shadowMapTable = CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, SRV_RG16F0(bufferIndex), get_srv_descriptor_size());
			shadowMapInfo.lightMatrix    = XMMatrixMultiply(viewMatrix, orthoMatrix);

			g_shadowMapper.render(
				device,
				commandQueue,
				commandAllocator,
				commandList,
				&g_uploadRingBuffer,
				cbPerFrameAddress,
				shadowMapInfo.lightMatrix,
				&CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RG16F0(bufferIndex), get_rtv_descriptor_size()),
				&CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, DSV_SHADOWMAP(bufferIndex), get_dsv_descriptor_size()),
				g_shadowmapRG16F[bufferIndex].get(),
				g_shadowmap32F[bufferIndex].get(),
				FUSE_SHADOW_MAPPING_VSM,
				staticObjects.first,
				staticObjects.second);

			
			// maybe add algorithm

			pShadowMapInfo = &shadowMapInfo;

		}

		g_deferredRenderer.render_light(
			device,
			commandQueue,
			commandAllocator,
			commandList,
			&g_uploadRingBuffer,
			g_srvHeap.get(),
			cbPerFrameAddress,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, SRV_GBUFFER(bufferIndex), get_srv_descriptor_size()),
			&finalRTV,
			gbuffer,
			currentLight,
			pShadowMapInfo);

	}

	/* Tonemap */

	g_tonemapper.run(device, commandQueue, commandAllocator, commandList,
		g_srvHeap.get(),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, SRV_RGBA16F2(bufferIndex), get_srv_descriptor_size()),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, UAV_RGBA8U0(bufferIndex), get_uav_descriptor_size()),
		g_fullresRGBA16F2[bufferIndex].get(),
		g_fullresRGBA8U0[bufferIndex].get(),
		get_screen_width(), get_screen_height());

	/* Finally copy to backbuffer */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	commandList.resource_barrier_transition(g_fullresRGBA8U0[bufferIndex].get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList.resource_barrier_transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(backBuffer, g_fullresRGBA8U0[bufferIndex].get());

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

	/* Draw text */

	std::stringstream ss;

	ss << "FPS: " << get_fps() << std::endl;

	g_textRenderer.render(
		device,
		commandQueue,
		commandAllocator,
		commandList,
		backBuffer,
		&rtvDescriptor,
		cbPerFrameAddress,
		&g_uploadRingBuffer,
		g_font.get(),
		ss.str().c_str(),
		XMFLOAT2(16, 16),
		XMFLOAT4(1, 1, 0, .25f));

	/* Draw GUI */

#ifdef FUSE_USE_EDITOR_GUI

	if (g_showGUI)
	{
		g_rocketInterface.render_begin(commandAllocator, commandList, cbPerFrameAddress, backBuffer, rtvDescriptor);
		g_editorGUI.render();
		g_rocketInterface.render_end();
	}
	
#endif

	/* Present */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	commandList.resource_barrier_transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

}

void renderer_application::on_configuration_init(application_config * configuration)
{
	configuration->syncInterval         = 0;
	configuration->presentFlags         = 0;
	configuration->swapChainBufferCount = NUM_BUFFERS;
}

bool renderer_application::create_shadow_map_buffers(ID3D12Device * device)
{

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = g_srvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_shadowmap32F[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R32_TYPELESS,
				SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.f, 0),
			IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"shadow_map_depth_buffer_32f_0");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels       = -1;

		device->CreateDepthStencilView(r.get(),
			&dsvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, DSV_SHADOWMAP(i), get_dsv_descriptor_size()));

	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_shadowmapRG16F[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R16G16_FLOAT,
				SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION,
				1, 1, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, reinterpret_cast<const float*>(&XMFLOAT4(0, 0, 0, 0))),
			IID_PPV_ARGS(&r)))
		{
			return false;
		}

		r->SetName(L"shadow_map_r16g16f0");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, RTV_RG16F0(i), get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, SRV_RG16F0(i), get_srv_descriptor_size()));

	}

	g_shadowMapper.set_viewport(make_fullscreen_viewport(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION));
	g_shadowMapper.set_scissor_rect(make_fullscreen_scissor_rect(SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION));

	return true;

}