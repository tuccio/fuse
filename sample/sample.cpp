#include <fuse/application.hpp>
#include <fuse/camera.hpp>
#include <fuse/fps_camera_controller.hpp>
#include <fuse/logger.hpp>
#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/gpu_upload_manager.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/mesh_manager.hpp>
#include <fuse/gpu_mesh_manager.hpp>

#include <d3dx12.h>
#include <DirectXMath.h>

#include <iostream>
#include <iterator>
#include <sstream>
#include <memory>

using namespace DirectX;

fuse::camera                                g_camera;
fuse::fps_camera_controller                 g_cameraController(&g_camera);
										    
fuse::com_ptr<ID3D12Resource>               g_constantBuffers;
D3D12_GPU_VIRTUAL_ADDRESS                   g_cameraBuffer;
void                                      * g_cameraBufferMap;
										    
fuse::com_ptr<ID3D12CommandAllocator>       g_commandAllocator;
fuse::com_ptr<ID3D12GraphicsCommandList>    g_commandList;
fuse::com_ptr<ID3D12PipelineState>          g_pipeLineState;
fuse::com_ptr<ID3D12RootSignature>          g_rootSignature;
										    
fuse::com_ptr<ID3D12DescriptorHeap>         g_dsvHeap;
										    
fuse::com_ptr<ID3D12Resource>               g_depthBuffer;
D3D12_CPU_DESCRIPTOR_HANDLE                 g_depthBufferView;
										    
std::shared_ptr<fuse::gpu_mesh>             g_triangleMesh;
										    
std::unique_ptr<fuse::gpu_upload_manager>   g_uploadManager;
										    
fuse::resource_factory                      g_resourceFactory;
std::unique_ptr<fuse::mesh_manager>         g_meshManager;
std::unique_ptr<fuse::gpu_mesh_manager>     g_gpuMeshManager;

struct camera_buffer_data
{
	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;
	XMMATRIX viewProjectionMatrix;
};

struct sample_application :
	fuse::application_base
{

	static bool load_triangle(void)
	{

		float triangleVertices[] = {
			0.f,  .5f, 0.f,
			.5f, -.5f, 0.f,
			-.5f, -.5f, 0.f
		};

		uint32_t triangleIndices[] = { 0, 1, 2 };

		static fuse::dynamic_resource_loader triangleLoader = {

			[](fuse::resource * r) // Loader
		{

			float triangleVertices[] = {
				0.f,  .5f, 0.f,
				.5f, -.5f, 0.f,
				-.5f, -.5f, 0.f
			};

			uint32_t triangleIndices[] = { 0, 1, 2 };

			fuse::mesh * m = static_cast<fuse::mesh*>(r);

			if (m->create(3, 1, FUSE_MESH_STORAGE_NONE))
			{
				std::memcpy(m->get_vertices(), triangleVertices, sizeof(triangleVertices));
				std::memcpy(m->get_indices(), triangleIndices, sizeof(triangleIndices));
				return true;
			}

			return false;

		},
			[](fuse::resource * r) // Unloader
		{
			static_cast<fuse::mesh*>(r)->clear();
		}

		};

		g_resourceFactory.create("mesh", "triangle", fuse::default_parameters(), &triangleLoader);
		g_triangleMesh = g_resourceFactory.create<fuse::gpu_mesh>("gpu_mesh", "triangle");

		return g_triangleMesh->load();

	}

	static bool on_device_created(ID3D12Device * device, ID3D12CommandQueue * commandQueue)
	{

		g_camera.look_at(XMFLOAT3(0, 0, -5), XMFLOAT3(0, 1, 0), XMFLOAT3(0, 0, 0));

		if (FUSE_HR_FAILED(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(camera_buffer_data)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&g_constantBuffers))) ||
			FUSE_HR_FAILED(g_constantBuffers->Map(0, nullptr, &g_cameraBufferMap)))
		{
			return false;
		}

		g_cameraBuffer = g_constantBuffers->GetGPUVirtualAddress();

		g_uploadManager = std::make_unique<fuse::gpu_upload_manager>(device, commandQueue);

		g_meshManager    = std::make_unique<fuse::mesh_manager>();
		g_gpuMeshManager = std::make_unique<fuse::gpu_mesh_manager>(device, g_uploadManager.get());

		g_resourceFactory.register_manager(g_meshManager.get());
		g_resourceFactory.register_manager(g_gpuMeshManager.get());

		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator)));
		FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.get(), nullptr, IID_PPV_ARGS(&g_commandList)));

		FUSE_HR_CHECK(g_commandAllocator->SetName(L"sample_command_allocator"));
		FUSE_HR_CHECK(g_commandList->SetName(L"sample_command_list"));
		FUSE_HR_CHECK(g_commandList->Close());

		fuse::com_ptr<ID3DBlob> testVS, testPS;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

		fuse::com_ptr<ID3DBlob> serializedSignature;
		fuse::com_ptr<ID3DBlob> errorsBlob;
		fuse::com_ptr<ID3D12ShaderReflection> shaderReflection;

		CD3DX12_ROOT_PARAMETER rootParameters[1];

		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		D3D12_DESCRIPTOR_HEAP_DESC  dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };

		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

		if (fuse::compile_shader("shaders/sample.hlsl", nullptr, "main_vs", "vs_5_0", compileFlags, &testVS) &&
			fuse::compile_shader("shaders/sample.hlsl", nullptr, "main_ps", "ps_5_0", compileFlags, &testPS) &&
			fuse::reflect_input_layout(testVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
			!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
			!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&g_rootSignature))) &&
			!FUSE_HR_FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvHeap))))
		{

			g_depthBufferView = g_dsvHeap->GetCPUDescriptorHandleForHeapStart();

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

			psoDesc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.NumRenderTargets              = 1;
			psoDesc.RTVFormats[0]                 = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.VS                            = { FUSE_BLOB_ARGS(testVS) };
			psoDesc.PS                            = { FUSE_BLOB_ARGS(testPS) };
			psoDesc.InputLayout                   = fuse::make_input_layout_desc(inputLayoutVector);
			psoDesc.pRootSignature                = g_rootSignature.get();
			psoDesc.SampleMask                    = UINT_MAX;
			psoDesc.SampleDesc                    = { 1, 0 };
			psoDesc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			if (FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipeLineState))))
			{
				return false;
			}

			if (!load_triangle())
			{
				return false;
			}

			return true;

		}

		return false;

	}

	static void on_device_released(ID3D12Device * device)
	{

		g_commandAllocator.reset();
		g_commandList.reset();

		g_rootSignature.reset();
		g_pipeLineState.reset();

		g_depthBuffer.reset();

		g_dsvHeap.reset();

		g_triangleMesh.reset();

		g_meshManager.reset();
		g_gpuMeshManager.reset();

		g_constantBuffers.reset();

		g_resourceFactory.clear();
		g_uploadManager.reset();

	}

	static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc)
	{

		FUSE_LOG("SwapChain", std::stringstream() << "Resized (" << desc->Width << "x" << desc->Height << ")");

		g_depthBuffer.reset();

		if (FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, desc->Width, desc->Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.f, 0),
			IID_PPV_ARGS(&g_depthBuffer))))
		{
			return false;
		}

		device->CreateDepthStencilView(g_depthBuffer.get(), nullptr, g_depthBufferView);

		g_cameraController.on_resize(desc->Width, desc->Height);

		return true;

	}

	static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam)
	{

		if (code == HC_ACTION)
		{

			g_cameraController.on_keyboard(wParam, lParam);

			if (KF_UP & lParam) std::cout << wParam << " released" << std::endl;

			/*float speed = .25f;

			if (!(KF_UP & lParam))
			{

				switch (wParam)
				{

				case 'W':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position + g_camera.forward() * speed);
					break;
				}

				case 'S':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position - g_camera.forward() * speed);
					break;
				}

				case 'A':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position - g_camera.right() * speed);
					break;
				}
				case 'D':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position + g_camera.right() * speed);
					break;
				}

				case 'E':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position + g_camera.up() * speed);
					break;
				}
				case 'Q':
				{
					auto position = g_camera.get_position();
					g_camera.set_position(position - g_camera.up() * speed);
					break;
				}

				default:

					return base_type::on_keyboard(code, wParam, lParam);

				}

				return 1;

			}*/

		}

		return base_type::on_keyboard(code, wParam, lParam);

	}

	static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam)
	{

		/*if (code == HC_ACTION)
		{

			static XMVECTOR lastPos;
			static bool rightButtonDown = false;

			MOUSEHOOKSTRUCT mouseStruct = * (PMOUSEHOOKSTRUCT) lParam;

			if (wParam == WM_RBUTTONDOWN)
			{
				lastPos = fuse::to_vector(XMFLOAT2(mouseStruct.pt.x, mouseStruct.pt.y));
				rightButtonDown = true;
			}
			else if (wParam == WM_RBUTTONUP)
			{
				rightButtonDown = false;
			}
			else if (wParam == WM_MOUSEMOVE && rightButtonDown)
			{

				XMVECTOR from = lastPos;
				XMVECTOR to   = fuse::to_vector(XMFLOAT2(mouseStruct.pt.x, mouseStruct.pt.y));

				XMVECTOR delta = XMVectorATan((to - from) / fuse::to_vector(XMFLOAT2(get_screen_width(), get_screen_height())));

				XMVECTOR r1 = XMQuaternionRotationAxis(fuse::to_vector(XMFLOAT3(0, 1, 0)), XMVectorGetX(delta));
				XMVECTOR r2 = XMQuaternionRotationAxis(fuse::to_vector(XMFLOAT3(1, 0, 0)), XMVectorGetY(delta));

				XMVECTOR newOrientation = XMQuaternionNormalize(XMQuaternionMultiply(XMQuaternionMultiply(r2, g_camera.get_orientation()), r1));

				g_camera.set_orientation(newOrientation);

				lastPos = to;

			}

		}*/

		g_cameraController.on_mouse(wParam, lParam);

		return base_type::on_mouse(code, wParam, lParam);

	}

	static void on_update(void)
	{

		std::stringstream ss;
		ss << "Sample (" << (int) get_fps() << " FPS)";
		SetWindowText(get_window(), ss.str().c_str());

		// TODO: proper timing
		g_cameraController.on_update(1.f);

	}

	static void on_render(ID3D12Device * device, ID3D12CommandQueue * commandQueue, ID3D12Resource * backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor)
	{

		/* Reset command list and command allocator */

		FUSE_HR_CHECK(g_commandAllocator->Reset());
		FUSE_HR_CHECK(g_commandList->Reset(g_commandAllocator.get(), nullptr));

		/* Setup the constant buffer */

		camera_buffer_data data;

		{

			XMMATRIX view       = g_camera.get_view_matrix();
			XMMATRIX projection = g_camera.get_projection_matrix();

			data.viewMatrix           = XMMatrixTranspose(view);
			data.projectionMatrix     = XMMatrixTranspose(projection);
			data.viewProjectionMatrix = XMMatrixMultiplyTranspose(view, projection);

			std::memcpy(g_cameraBufferMap, &data, sizeof(data));

		}
		

		/* Prepare to render on back buffer */

		g_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer,
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET));

		/* State changes */

		g_commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

		g_commandList->SetPipelineState(g_pipeLineState.get());

		g_commandList->SetGraphicsRootSignature(g_rootSignature.get());
		g_commandList->SetGraphicsRootConstantBufferView(0, g_cameraBuffer);
		set_screen_viewport(g_commandList.get());

		g_commandList->ClearRenderTargetView(rtvDescriptor, (const float *) &XMFLOAT4(0, 0, 0, 0), 0, nullptr);

		/* Draw */

		g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		g_commandList->IASetVertexBuffers(0, 1, g_triangleMesh->get_vertex_buffers());
		g_commandList->IASetIndexBuffer(&g_triangleMesh->get_index_data());

		g_commandList->DrawIndexedInstanced(g_triangleMesh->get_num_indices(), 1, 0, 0, 0);

		/* Prepare to present */

		g_commandList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT));

		/* Issue the command list to the queue */

		FUSE_HR_CHECK(g_commandList->Close());

		ID3D12CommandList * commandLists[] = { g_commandList.get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

	}

	static void on_configuration_init(fuse::application_config * configuration)
	{
		configuration->syncInterval = 0;
		configuration->presentFlags = 0;
	}

};

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	fuse::logger logger(&std::cout);

	using app = fuse::application<sample_application>;

	if (app::init(hInstance) &&
	    app::create_window(1280, 720, "Sample") &&
	    app::create_pipeline(D3D_FEATURE_LEVEL_11_0, true))
	{
		app::main_loop();
	}

	app::shutdown();

	return 0;

}