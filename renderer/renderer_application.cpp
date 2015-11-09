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

#include <fuse/camera.hpp>
#include <fuse/fps_camera_controller.hpp>

#include "cbuffer_structs.hpp"
#include "deferred_renderer.hpp"
#include "scene.hpp"
#include "tonemapper.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define NUM_BUFFERS      (2u)
#define NUM_RTVS         (4u)
#define NUM_SRVS_UAVS    (6u)
#define UPLOAD_HEAP_SIZE (1 << 20)

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
									   
com_ptr<ID3D12Resource>   g_cbPerFrame;
D3D12_GPU_VIRTUAL_ADDRESS g_cbPerFrameAddress;

gpu_ring_buffer    g_uploadRingBuffer;
gpu_upload_manager g_uploadManager;
								   	   
//com_ptr<ID3D12Resource>                g_fullresRGBA16F[2];
//com_ptr<ID3D12Resource>                g_fullresRGBA8U;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F0;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F1;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F2;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA8U;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_depthBuffer;

std::unique_ptr<assimp_loader> g_sceneLoader;
								       
resource_factory                     g_resourceFactory;
std::unique_ptr<image_manager>       g_imageManager;
std::unique_ptr<mesh_manager>        g_meshManager;
std::unique_ptr<material_manager>    g_materialManager;
std::unique_ptr<gpu_mesh_manager>    g_gpuMeshManager;
std::unique_ptr<texture_manager>     g_textureManager;
std::unique_ptr<bitmap_font_manager> g_bitmapFontManager;

std::unique_ptr<fps_camera_controller> g_cameraController;

bitmap_font_ptr    g_font;

scene              g_scene;
deferred_renderer  g_deferredRenderer;
tonemapper         g_tonemapper;
text_renderer      g_textRenderer;

gpu_global_resource_state g_globalResourceState;

bool renderer_application::on_device_created(ID3D12Device * device, gpu_command_queue & commandQueue)
{


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

	auto light = g_scene.get_lights_iterators().first;

	light->direction = XMFLOAT3(-0.281581f, 0.522937f, -0.804518f);
	light->luminance = to_float3(to_vector(light->luminance) * .2f);
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

	D3D12_DESCRIPTOR_HEAP_DESC gbufferDSVDesc = { };

	gbufferDSVDesc.NumDescriptors = NUM_BUFFERS;
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

	if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cb_per_frame)),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			nullptr,
			IID_PPV_ARGS(&g_cbPerFrame)))
	{
		return false;
	}

	g_cbPerFrameAddress = g_cbPerFrame->GetGPUVirtualAddress();

	/*g_font = resource_factory::get_singleton_pointer()->create<bitmap_font>("bitmap_font", "fonts/consolas_regular_14");

	if (!g_font->load())
	{
		return false;
	}*/

	/* Initialize renderer */

	return g_deferredRenderer.init(device) &&
	       g_tonemapper.init(device) &&
	       g_textRenderer.init(device);

}

void renderer_application::on_device_released(ID3D12Device * device)
{

	std::for_each(g_commandAllocator.begin(), g_commandAllocator.end(), [](auto & r) { r.reset(); });
	std::for_each(g_commandList.begin(), g_commandList.end(), [](auto & r) { r.reset(); });

	std::for_each(g_fullresRGBA16F0.begin(), g_fullresRGBA16F0.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA16F1.begin(), g_fullresRGBA16F1.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA16F2.begin(), g_fullresRGBA16F2.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA8U.begin(), g_fullresRGBA8U.end(), [](auto & r) { r.reset(); });
	std::for_each(g_depthBuffer.begin(), g_depthBuffer.end(), [](auto & r) { r.reset(); });

	g_rtvHeap.reset();
	g_dsvHeap.reset();

	g_imageManager.reset();
	g_meshManager.reset();
	g_gpuMeshManager.reset();

	g_resourceFactory.clear();

}

bool renderer_application::on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc)
{

	//g_deferredRenderer.on_resize(desc->Width, desc->Height);

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

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_depthBuffer[i];

		r.reset();

		if (!gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(
					DXGI_FORMAT_R24G8_TYPELESS,
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

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

		dsvDesc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

		srvDesc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels       = -1;

		device->CreateDepthStencilView(r.get(),
			&dsvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, i, get_dsv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			&srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i + 3, get_srv_descriptor_size()));

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

		r->SetName(L"fullres_rgba16f_rt_0");

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		
		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_RTVS * i, get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i, get_srv_descriptor_size()));
		
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

		r->SetName(L"fullres_rgba16f_rt_1");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_RTVS * i + 1, get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i + 1, get_srv_descriptor_size()));


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

		r->SetName(L"fullres_rgba16f_rt_2");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_RTVS * i + 3, get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i + 4, get_srv_descriptor_size()));


	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U[i];

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

		r->SetName(L"fullres_rgba8u_rt");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_RTVS * i + 2, get_rtv_descriptor_size()));

		device->CreateShaderResourceView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i + 2, get_srv_descriptor_size()));

		device->CreateUnorderedAccessView(r.get(),
			nullptr, nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(srvHandle, NUM_SRVS_UAVS * i + 5, get_uav_descriptor_size()));

	}

	return true;

}

LRESULT CALLBACK renderer_application::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
{

	if (!code)
	{

		if (wParam == VK_F11 && ((KF_UP << 16) & lParam) == 0)
		{
			bool goFullscreen = !is_fullscreen();
			set_fullscreen(goFullscreen);
			set_cursor(goFullscreen, goFullscreen);
			g_cameraController->set_auto_center_mouse(goFullscreen);
		}
		else
		{
			g_cameraController->on_keyboard(wParam, lParam);
		}

	}

	return base_type::on_keyboard(code, wParam, lParam);

}

LRESULT CALLBACK renderer_application::on_mouse(int code, WPARAM wParam, LPARAM lParam)
{

	if (!code)
	{
		g_cameraController->on_mouse(wParam, lParam);
	}

	return base_type::on_mouse(code, wParam, lParam);

}

void renderer_application::on_update(float dt)
{

	//FUSE_LOG("Time", std::to_string(dt).c_str());

	std::stringstream ss;
	ss << "Fuse Renderer (" << (int) get_fps() << " FPS)";

	std::string title = ss.str();
	SetWindowText(get_window(), title.c_str());

	//FUSE_LOG("FPS", title.c_str());

	g_cameraController->on_update(dt);

}

void renderer_application::upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue)
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
	cbPerFrame.screen.orthoProjection   = XMMatrixTranspose(XMMatrixOrthographicOffCenterLH(0, get_screen_width(), 0, get_screen_height(), 0.f, 1.f));

	D3D12_GPU_VIRTUAL_ADDRESS address;
	D3D12_GPU_VIRTUAL_ADDRESS heapAddress = g_uploadRingBuffer.get_heap()->GetGPUVirtualAddress();

	void * cbData = g_uploadRingBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_frame), &address);
	memcpy(cbData, &cbPerFrame, sizeof(cb_per_frame));
	g_uploadManager.upload_buffer(commandQueue, g_cbPerFrame.get(), 0, g_uploadRingBuffer.get_heap(), address - heapAddress, sizeof(cb_per_frame));

	/*g_uploadManager->upload(
		device,
		g_cbPerFrame.get(),
		0, 1,
		&cbPerFrameData,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_cbPerFrame.get(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			D3D12_RESOURCE_STATE_COPY_DEST),
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_cbPerFrame.get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));*/

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

	/* Setup render */

	upload_per_frame_resources(device, commandQueue);

	g_deferredRenderer.render_init(&g_scene);

	/* Clear the buffers */

	CD3DX12_CPU_DESCRIPTOR_HANDLE gbufferHandles[] = {
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_RTVS, get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_RTVS + 1, get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_RTVS + 2, get_rtv_descriptor_size())
	};

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(g_dsvHandle, bufferIndex, get_dsv_descriptor_size());
	
	float clearColor[4] = { 0.f };

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	commandList->ClearRenderTargetView(gbufferHandles[0], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbufferHandles[1], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbufferHandles[2], clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	commandList.resource_barrier_transition(g_cbPerFrame.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	FUSE_HR_CHECK(commandList->Close());
	commandQueue.execute(commandList);

	/* GBuffer rendering */

	ID3D12Resource * gbuffer[] = { g_fullresRGBA16F0[bufferIndex].get(), g_fullresRGBA16F1[bufferIndex].get(), g_fullresRGBA8U[bufferIndex].get(), g_depthBuffer[bufferIndex].get() };

	g_deferredRenderer.render_gbuffer(
		device,
		commandQueue,
		commandAllocator,
		commandList,
		&g_uploadRingBuffer,
		g_cbPerFrameAddress,
		gbufferHandles,
		&dsvHandle,
		gbuffer);

	/* Prepare for shading */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	CD3DX12_CPU_DESCRIPTOR_HANDLE finalRTV(g_rtvHandle, bufferIndex * NUM_RTVS + 3, get_rtv_descriptor_size());

	commandList.resource_barrier_transition(g_fullresRGBA16F2[bufferIndex].get(),D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(finalRTV, clearColor, 0, nullptr);

	FUSE_HR_CHECK(commandList->Close());

	commandQueue.execute(commandList);

	/* Shading */

	auto lightsIterators = g_scene.get_lights_iterators();

	for (auto it = lightsIterators.first; it != lightsIterators.second; it++)
	{

		g_deferredRenderer.render_light(
			device,
			commandQueue,
			commandAllocator,
			commandList,
			&g_uploadRingBuffer,
			g_srvHeap.get(),
			g_cbPerFrameAddress,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, bufferIndex * NUM_SRVS_UAVS, get_srv_descriptor_size()),
			&finalRTV,
			gbuffer,
			&*it);

	}

	/* Tonemap */

	g_tonemapper.run(device, commandQueue, commandAllocator, commandList,
		g_srvHeap.get(),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, bufferIndex * NUM_SRVS_UAVS + 4, get_srv_descriptor_size()),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(g_srvHandle, bufferIndex * NUM_SRVS_UAVS + 5, get_uav_descriptor_size()),
		g_fullresRGBA16F2[bufferIndex].get(),
		g_fullresRGBA8U[bufferIndex].get(),
		get_screen_width(), get_screen_height());

	/* Finally copy to backbuffer and prepare to present */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	commandList.resource_barrier_transition(g_fullresRGBA8U[bufferIndex].get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList.resource_barrier_transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(backBuffer, g_fullresRGBA8U[bufferIndex].get());

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