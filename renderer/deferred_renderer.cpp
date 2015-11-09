#include "deferred_renderer.hpp"

#include <algorithm>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <d3dx12.h>

#include "cbuffer_structs.hpp"

using namespace fuse;

deferred_renderer::deferred_renderer(void) { }

bool deferred_renderer::init(ID3D12Device * device)
{

	return create_psos(device);

}

void deferred_renderer::shutdown(void)
{

}

void deferred_renderer::render_init(scene * scene)
{

	m_staticObjects.clear();

	auto staticObjectsIterators = scene->get_static_objects_iterators();

	// TODO: Culling

	for (auto it = staticObjectsIterators.first; it != staticObjectsIterators.second; it++)
	{
		m_staticObjects.push_back(&*it);
	}

	m_camera = scene->get_active_camera();

	assert(m_camera && "No active camera in the scene");

	// TODO: Maybe sort by material

}

void deferred_renderer::render_gbuffer(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	ID3D12CommandAllocator * commandAllocator,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer * ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const D3D12_CPU_DESCRIPTOR_HANDLE * gbuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE * dsv,
	ID3D12Resource ** gbufferResources)
{

	auto view              = m_camera->get_view_matrix();
	auto projection        = m_camera->get_projection_matrix();
	auto viewProjection    = XMMatrixMultiply(view, projection);
	auto invViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProjection), viewProjection);

	/* Setup the pipeline state */

	FUSE_HR_CHECK(commandList->Reset(commandAllocator, m_gbufferPSO.get()));

	commandList.resource_barrier_transition(gbufferResources[0], D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbufferResources[1], D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbufferResources[2], D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbufferResources[3], D3D12_RESOURCE_STATE_DEPTH_WRITE);

	commandList->OMSetRenderTargets(3, gbuffer, false, dsv);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootSignature(m_gbufferRS.get());

	commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

	/* Render loop */

	material defaultMaterial;
	defaultMaterial.set_default();

	UINT fenceValue = 0;

	for (auto renderable : m_staticObjects)
	{

		/* Fill buffer per object */

		auto world = renderable->get_world_matrix();;

		cb_per_object cbPerObject;

		cbPerObject.transform.world               = XMMatrixTranspose(world);
		cbPerObject.transform.worldView           = XMMatrixTranspose(XMMatrixMultiply(world, view));
		cbPerObject.transform.worldViewProjection = XMMatrixTranspose(XMMatrixMultiply(world, viewProjection));

		auto objectMaterial = renderable->get_material();

		material * materialData = (objectMaterial && objectMaterial->load()) ? objectMaterial.get() : &defaultMaterial;

		cbPerObject.material.baseColor = reinterpret_cast<const XMFLOAT3&>(materialData->get_base_albedo());
		cbPerObject.material.roughness = materialData->get_roughness();
		cbPerObject.material.specular  = materialData->get_specular();
		cbPerObject.material.metallic  = materialData->get_metallic();

		D3D12_GPU_VIRTUAL_ADDRESS address;

		void * cbData = ringBuffer->allocate_constant_buffer(device, commandQueue, sizeof(cb_per_object), &address);

		if (cbData)
		{

			memcpy(cbData, &cbPerObject, sizeof(cb_per_object));

			commandList->SetGraphicsRootConstantBufferView(1, address);

			/* Draw */

			auto mesh = renderable->get_mesh();

			if (mesh && mesh->load())
			{

				commandList.resource_barrier_transition(mesh->get_resource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

				commandList->IASetVertexBuffers(0, 2, mesh->get_vertex_buffers());
				commandList->IASetIndexBuffer(&mesh->get_index_data());

				commandList->DrawIndexedInstanced(mesh->get_num_indices(), 1, 0, 0, 0);

			}			

		}

	}

	commandList->Close();

	commandQueue.execute(commandList);

}

void deferred_renderer::render_light(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	ID3D12CommandAllocator * commandAllocator,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer * ringBuffer,
	ID3D12DescriptorHeap * gbufferHeap,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const D3D12_GPU_DESCRIPTOR_HANDLE & gbufferSRVDescTable,
	const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
	ID3D12Resource ** gbufferResources,
	const light * light)
{

	D3D12_GPU_VIRTUAL_ADDRESS address;
	void * cbData = ringBuffer->allocate_constant_buffer(device, commandQueue, sizeof(cb_per_light), &address);

	if (cbData)
	{

		cb_per_light cbPerLight = { };

		//cbPerLight.light.type      = light->type;
		cbPerLight.light.direction = light->direction;
		cbPerLight.light.luminance = light->luminance;
		cbPerLight.light.ambient   = light->ambient;

		memcpy(cbData, &cbPerLight, sizeof(cb_per_light));

		FUSE_HR_CHECK(commandList->Reset(commandAllocator, m_shadingPSO.get()));

		commandList.resource_barrier_transition(gbufferResources[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbufferResources[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbufferResources[2], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbufferResources[3], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->SetGraphicsRootSignature(m_shadingRS.get());

		commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);
		commandList->SetGraphicsRootConstantBufferView(1, address);
		
		commandList->SetDescriptorHeaps(1, &gbufferHeap);
		commandList->SetGraphicsRootDescriptorTable(2, gbufferSRVDescTable);

		commandList->OMSetRenderTargets(1, rtv, false, nullptr);

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);

		FUSE_HR_CHECK(commandList->Close());
		
		//commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);

		commandQueue.execute(commandList);

	}

}

bool deferred_renderer::create_psos(ID3D12Device * device)
{
	return create_gbuffer_pso(device) &&
		create_shading_pso(device);
}

bool deferred_renderer::create_gbuffer_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> gbufferVS, gbufferPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (compile_shader("shaders/deferred_shading_gbuffer.hlsl", nullptr, "gbuffer_vs", "vs_5_0", compileFlags, &gbufferVS) &&
		compile_shader("shaders/deferred_shading_gbuffer.hlsl", nullptr, "gbuffer_ps", "ps_5_0", compileFlags, &gbufferPS) &&
		reflect_input_layout(gbufferVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_gbufferRS))))
	{

		// Adjust the input desc since gpu_mesh uses slot 0 for position and
		// slot 1 for non position data (tangent space vectors, texcoords, ...)

		for (auto & elementDesc : inputLayoutVector)
		{
			if (strcmp(elementDesc.SemanticName, "POSITION"))
			{
				elementDesc.InputSlot = 1;
			}
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

		psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets         = 3;
		psoDesc.RTVFormats[0]            = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.RTVFormats[1]            = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.RTVFormats[2]            = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat                = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.VS                       = { FUSE_BLOB_ARGS(gbufferVS) };
		psoDesc.PS                       = { FUSE_BLOB_ARGS(gbufferPS) };
		psoDesc.InputLayout              = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature           = m_gbufferRS.get();
		psoDesc.SampleMask               = UINT_MAX;
		psoDesc.SampleDesc               = { 1, 0 };
		psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_gbufferPSO)));

	}

	return false;

}

bool deferred_renderer::create_shading_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> quadVS, finalPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[3];

	CD3DX12_DESCRIPTOR_RANGE srv[1];

	srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(_countof(srv), srv, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (compile_shader("shaders/quad_vs.hlsl", nullptr, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
		compile_shader("shaders/deferred_shading_final.hlsl", nullptr, "shading_ps", "ps_5_0", compileFlags, &finalPS) &&
		reflect_input_layout(quadVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_shadingRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

		psoDesc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		psoDesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_ONE;
		psoDesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState                      = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable          = FALSE;
		psoDesc.NumRenderTargets                       = 1;
		psoDesc.RTVFormats[0]                          = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.VS                                     = { FUSE_BLOB_ARGS(quadVS) };
		psoDesc.PS                                     = { FUSE_BLOB_ARGS(finalPS) };
		psoDesc.InputLayout                            = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                         = m_shadingRS.get();
		psoDesc.SampleMask                             = UINT_MAX;
		psoDesc.SampleDesc                             = { 1, 0 };
		psoDesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_shadingPSO)));

	}

	return false;

}