#include "renderer_application.hpp"

#include <fuse/logger.hpp>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <fuse/gpu_upload_manager.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/image_manager.hpp>
#include <fuse/material_manager.hpp>
#include <fuse/mesh_manager.hpp>
#include <fuse/gpu_mesh_manager.hpp>

#include <fuse/assimp_loader.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include <fuse/camera.hpp>
#include <fuse/fps_camera_controller.hpp>

#include "cbuffer_structs.hpp"
#include "deferred_renderer.hpp"
#include "scene.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>

using namespace fuse;

#define NUM_BUFFERS      (4u)
#define NUM_GBUFFER_RTVS (3u)

std::array<com_ptr<ID3D12CommandAllocator>,    NUM_BUFFERS> g_commandAllocator;
std::array<com_ptr<ID3D12GraphicsCommandList>, NUM_BUFFERS> g_commandList;
								       
com_ptr<ID3D12DescriptorHeap> g_dsvHeap;
D3D12_CPU_DESCRIPTOR_HANDLE   g_dsvHandle;
								       
com_ptr<ID3D12DescriptorHeap> g_rtvHeap;
D3D12_CPU_DESCRIPTOR_HANDLE   g_rtvHandle;
									   
com_ptr<ID3D12Resource> g_cbPerFrame;

gpu_ring_buffer                     g_uploadRingBuffer;
std::unique_ptr<gpu_upload_manager> g_uploadManager;
								   	   
//com_ptr<ID3D12Resource>                g_fullresRGBA16F[2];
//com_ptr<ID3D12Resource>                g_fullresRGBA8U;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F0;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA16F1;
std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_fullresRGBA8U;

std::array<com_ptr<ID3D12Resource>, NUM_BUFFERS> g_depthBuffer;

std::unique_ptr<assimp_loader>    g_sceneLoader;
								       
resource_factory                  g_resourceFactory;
std::unique_ptr<image_manager>    g_imageManager;
std::unique_ptr<mesh_manager>     g_meshManager;
std::unique_ptr<material_manager> g_materialManager;
std::unique_ptr<gpu_mesh_manager> g_gpuMeshManager;

std::unique_ptr<fps_camera_controller> g_cameraController;

scene              g_scene;
deferred_renderer  g_deferredRenderer;

bool renderer_application::on_device_created(ID3D12Device * device, ID3D12CommandQueue * commandQueue)
{


	/* Resource managers */

	g_uploadManager = std::make_unique<gpu_upload_manager>(device, commandQueue);

	g_imageManager    = std::make_unique<image_manager>();
	g_materialManager = std::make_unique<material_manager>();
	g_meshManager     = std::make_unique<mesh_manager>();
	g_gpuMeshManager  = std::make_unique<gpu_mesh_manager>(device, g_uploadManager.get());

	g_resourceFactory.register_manager(g_imageManager.get());
	g_resourceFactory.register_manager(g_materialManager.get());
	g_resourceFactory.register_manager(g_meshManager.get());
	g_resourceFactory.register_manager(g_gpuMeshManager.get());

	/* Load scene */

	//g_sceneLoader = std::make_unique<assimp_loader>("scene/default.fbx");
	g_sceneLoader = std::make_unique<assimp_loader>("scene/cornellbox.fbx");

	//g_sceneLoader = std::make_unique<assimp_loader>("scene/sponza.fbx", ppsteps);

	if (!g_scene.import_static_objects(g_sceneLoader.get()) ||
		!g_scene.import_cameras(g_sceneLoader.get()))
	{
		return false;
	}

	g_scene.get_active_camera()->set_zfar(10000.f);

	g_cameraController = std::make_unique<fps_camera_controller>(g_scene.get_active_camera());
	g_cameraController->set_speed(XMFLOAT3(15, 15, 15));

	/* Prepare the command list */

	for (unsigned int bufferIndex = 0; bufferIndex < NUM_BUFFERS; bufferIndex++)
	{
		
		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator[bufferIndex])));
		FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[bufferIndex].get(), nullptr, IID_PPV_ARGS(&g_commandList[bufferIndex])));

		FUSE_HR_CHECK(g_commandAllocator[bufferIndex]->SetName(L"main_command_allocator"));
		FUSE_HR_CHECK(g_commandList[bufferIndex]->SetName(L"main_command_list"));

		FUSE_HR_CHECK(g_commandList[bufferIndex]->Close());

	}	

	/* Create descriptor heaps */

	D3D12_DESCRIPTOR_HEAP_DESC gbufferRTVDesc = { };

	gbufferRTVDesc.NumDescriptors = NUM_GBUFFER_RTVS * NUM_BUFFERS;
	gbufferRTVDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	gbufferRTVDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	D3D12_DESCRIPTOR_HEAP_DESC gbufferDSVDesc = { };

	gbufferDSVDesc.NumDescriptors = NUM_BUFFERS;
	gbufferDSVDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	gbufferDSVDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FUSE_HR_FAILED(device->CreateDescriptorHeap(&gbufferRTVDesc, IID_PPV_ARGS(&g_rtvHeap))) ||
	    FUSE_HR_FAILED(device->CreateDescriptorHeap(&gbufferDSVDesc, IID_PPV_ARGS(&g_dsvHeap))))
	{
		return false;
	}

	/* Create constant buffers */

	if (!g_uploadRingBuffer.create(device, &CD3DX12_HEAP_DESC(1 << 20, D3D12_HEAP_TYPE_UPLOAD, 0, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS)))
	{
		return false;
	}

	if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(cb_per_frame)),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			nullptr,
			IID_PPV_ARGS(&g_cbPerFrame))))
	{
		return false;
	}

	/* Initialize renderer */

	g_deferredRenderer.set_debug(true); // TODO: Remove

	return g_deferredRenderer.init(device, g_cbPerFrame.get(), g_uploadManager.get());

}

void renderer_application::on_device_released(ID3D12Device * device)
{

	std::for_each(g_commandAllocator.begin(), g_commandAllocator.end(), [](auto & r) { r.reset(); });
	std::for_each(g_commandList.begin(), g_commandList.end(), [](auto & r) { r.reset(); });

	std::for_each(g_fullresRGBA16F0.begin(), g_fullresRGBA16F0.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA16F1.begin(), g_fullresRGBA16F1.end(), [](auto & r) { r.reset(); });
	std::for_each(g_fullresRGBA8U.begin(), g_fullresRGBA8U.end(), [](auto & r) { r.reset(); });
	std::for_each(g_depthBuffer.begin(), g_depthBuffer.end(), [](auto & r) { r.reset(); });

	g_rtvHeap.reset();
	g_dsvHeap.reset();

	g_imageManager.reset();
	g_meshManager.reset();
	g_gpuMeshManager.reset();

	g_resourceFactory.clear();
	g_uploadManager.reset();

}

bool renderer_application::on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc)
{

	g_deferredRenderer.on_resize(desc->Width, desc->Height);
	g_cameraController->on_resize(desc->Width, desc->Height);

	camera * mainCamera = g_scene.get_active_camera();

	if (mainCamera)
	{
		mainCamera->set_aspect_ratio((float) desc->Width / desc->Height);
	}

	g_dsvHandle = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_depthBuffer[i];

		r.reset();


		if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				desc->Width, desc->Height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0),
			IID_PPV_ARGS(&r))))
		{
			return false;
		}

		r->SetName(L"main_depth_buffer");

		device->CreateDepthStencilView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, i, get_dsv_descriptor_size()));

	}

	g_rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	const float black[4] = { 0 };

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F0[i];
		
		r.reset();

		if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				desc->Width,
				desc->Height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
			IID_PPV_ARGS(&r))))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f_rtv_0");
		
		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_GBUFFER_RTVS * i, get_rtv_descriptor_size()));
		
	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA16F1[i];

		r.reset();

		if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R16G16B16A16_FLOAT,
				desc->Width,
				desc->Height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, black),
			IID_PPV_ARGS(&r))))
		{
			return false;
		}

		r->SetName(L"fullres_rgba16f_rtv_1");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_GBUFFER_RTVS * i + 1, get_rtv_descriptor_size()));


	}

	for (int i = 0; i < NUM_BUFFERS; i++)
	{

		auto & r = g_fullresRGBA8U[i];

		r.reset();

		if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_R8G8B8A8_UNORM,
				desc->Width,
				desc->Height,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, black),
			IID_PPV_ARGS(&r))))
		{
			return false;
		}

		r->SetName(L"fullres_rgba8u_rtv");

		device->CreateRenderTargetView(r.get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, NUM_GBUFFER_RTVS * i + 2, get_rtv_descriptor_size()));

	}

	return true;

}

LRESULT CALLBACK renderer_application::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
{

	if (!code)
	{
		g_cameraController->on_keyboard(wParam, lParam);
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

void renderer_application::on_update(void)
{

	std::stringstream ss;
	ss << "Fuse Renderer (" << (int) get_fps() << " FPS)";
	SetWindowText(get_window(), ss.str().c_str());

	g_cameraController->on_update(1.f);

	g_uploadManager->collect_garbage();

}

void renderer_application::on_render(ID3D12Device * device, gpu_command_queue & commandQueue, ID3D12Resource * backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor, UINT bufferIndex)
{

	/* Reset command list and command allocator */

	ID3D12CommandAllocator    * commandAllocator = g_commandAllocator[bufferIndex].get();
	ID3D12GraphicsCommandList * commandList      = g_commandList[bufferIndex].get();

	FUSE_HR_CHECK(commandAllocator->Reset());
	FUSE_HR_CHECK(commandList->Reset(commandAllocator, nullptr));

	/* Setup render */

	g_deferredRenderer.render_init(&g_scene);
	set_screen_viewport(commandList);

	/* Render gbuffer */

	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA16F[0].get(),
			g_fullresRGBA16F0[bufferIndex].get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA16F[1].get(),
			g_fullresRGBA16F1[bufferIndex].get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA8U.get(),
			g_fullresRGBA8U[bufferIndex].get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE gbufferHandles[] = {
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_GBUFFER_RTVS, get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_GBUFFER_RTVS + 1, get_rtv_descriptor_size()),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHandle, bufferIndex * NUM_GBUFFER_RTVS + 2, get_rtv_descriptor_size())
	};

	FUSE_HR_CHECK(commandList->Close());
	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList **) &commandList);

	g_deferredRenderer.render_gbuffer(device, commandQueue, commandAllocator, commandList, g_uploadRingBuffer, gbufferHandles, &CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHandle, bufferIndex, get_dsv_descriptor_size()));

	/* Resource barrier for gbuffer */

	commandList->Reset(commandAllocator, nullptr);

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA16F[0].get(),
			g_fullresRGBA16F0[bufferIndex].get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE));

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA16F[1].get(),
			g_fullresRGBA16F1[bufferIndex].get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE));

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			//g_fullresRGBA8U.get(),
			g_fullresRGBA8U[bufferIndex].get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE));

	/* Copy gbuffer to back buffer to debug */

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyResource(backBuffer, g_fullresRGBA8U[bufferIndex].get());

	commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PRESENT));

	FUSE_HR_CHECK(commandList->Close());
	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList **) &commandList);

}

void renderer_application::on_configuration_init(fuse::application_config * configuration)
{
	configuration->syncInterval         = 0;
	configuration->presentFlags         = 0;
	configuration->swapChainBufferCount = NUM_BUFFERS;
}