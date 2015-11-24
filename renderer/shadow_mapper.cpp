#include "shadow_mapper.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

using namespace fuse;

bool shadow_mapper::init(ID3D12Device * device, const shadow_mapper_configuration & cfg)
{
	m_configuration = cfg;
	return create_psos(device);
}

void shadow_mapper::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer * ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const XMMATRIX & lightMatrix,
	const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
	const D3D12_CPU_DESCRIPTOR_HANDLE * dsv,
	ID3D12Resource * renderTarget,
	ID3D12Resource * depthBuffer,
	shadow_mapping_algorithm algorithm,
	renderable_iterator begin,
	renderable_iterator end)
{

	/* Setup the pipeline state */

	ID3D12PipelineState * pso;

	switch (algorithm)
	{
	case FUSE_SHADOW_MAPPING_NONE:
		return;
	case FUSE_SHADOW_MAPPING_VSM:
		pso = m_vsmPSO.get();
		break;
	case FUSE_SHADOW_MAPPING_EVSM2:
		pso = m_evsm2PSO.get();
		break;
	default:
		pso = m_regularShadowMapPSO.get();
		break;
	}

	commandList.reset_command_list(pso);

	commandList.resource_barrier_transition(depthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	commandList->ClearDepthStencilView(*dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	if (!renderTarget)
	{
		commandList->OMSetRenderTargets(0, nullptr, false, dsv);
	}
	else
	{

		float black[4] = { 0 };

		commandList.resource_barrier_transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ClearRenderTargetView(*rtv, black, 0, nullptr);
		commandList->OMSetRenderTargets(1, rtv, false, dsv);

	}

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootSignature(m_shadowMapRS.get());
	commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

	/* Render loop */

	for (auto it = begin; it != end; it++)
	{

		D3D12_GPU_VIRTUAL_ADDRESS cbPerLight;

		void * cbData = ringBuffer->allocate_constant_buffer(device, commandQueue, sizeof(XMMATRIX), &cbPerLight);

		if (cbData)
		{

			renderable * object = *it;

			XMMATRIX worldLightSpace = XMMatrixMultiplyTranspose(object->get_world_matrix(), lightMatrix);
			memcpy(cbData, &worldLightSpace, sizeof(XMMATRIX));

			/* Draw */

			auto mesh = object->get_mesh();

			if (mesh && mesh->load())
			{

				commandList->SetGraphicsRootConstantBufferView(1, cbPerLight);

				commandList.resource_barrier_transition(mesh->get_resource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

				commandList->IASetVertexBuffers(0, 1, mesh->get_vertex_buffers());
				commandList->IASetIndexBuffer(&mesh->get_index_data());

				commandList->DrawIndexedInstanced(mesh->get_num_indices(), 1, 0, 0, 0);

			}

		}

	}

	FUSE_HR_CHECK(commandList->Close());

	commandQueue.execute(commandList);

}

bool shadow_mapper::create_psos(ID3D12Device * device)
{

	return create_rs(device) &&
		create_regular_pso(device) &&
		create_vsm_pso(device) &&
		create_evsm2_pso(device);

}

bool shadow_mapper::create_rs(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return !FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_shadowMapRS)));

}

bool shadow_mapper::create_regular_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> shadowMapVS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	UINT compileFlags = 0;

	if (compile_shader("shaders/shadow_mapping.hlsl", nullptr, "shadow_map_vs", "vs_5_0", compileFlags, &shadowMapVS) &&
		reflect_input_layout(shadowMapVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = m_configuration.cullMode;
		psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets         = 0;
		psoDesc.DSVFormat                = m_configuration.dsvFormat;
		psoDesc.VS                       = { FUSE_BLOB_ARGS(shadowMapVS) };
		psoDesc.InputLayout              = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature           = m_shadowMapRS.get();
		psoDesc.SampleMask               = UINT_MAX;
		psoDesc.SampleDesc               = { 1, 0 };
		psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_regularShadowMapPSO)));

	}

	return false;

}

bool shadow_mapper::create_vsm_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> shadowMapVS, vsmPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3D12ShaderReflection> shaderReflection;

	UINT compileFlags = 0;

	if (compile_shader("shaders/shadow_mapping.hlsl", nullptr, "shadow_map_vs", "vs_5_0", compileFlags, &shadowMapVS) &&
		compile_shader("shaders/shadow_mapping.hlsl", nullptr, "vsm_ps", "ps_5_0", compileFlags, &vsmPS) &&
		reflect_input_layout(shadowMapVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode           = m_configuration.cullMode;
		psoDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets                   = 1;
		psoDesc.RTVFormats[0]                      = m_configuration.evsm2Format;
		psoDesc.DSVFormat                          = m_configuration.dsvFormat;
		psoDesc.VS                                 = { FUSE_BLOB_ARGS(shadowMapVS) };
		psoDesc.PS                                 = { FUSE_BLOB_ARGS(vsmPS) };
		psoDesc.InputLayout                        = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                     = m_shadowMapRS.get();
		psoDesc.SampleMask                         = UINT_MAX;
		psoDesc.SampleDesc                         = { 1, 0 };
		psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_vsmPSO)));

	}

	return false;

}

bool shadow_mapper::create_evsm2_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> shadowMapVS, evsm2PS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3D12ShaderReflection> shaderReflection;

	UINT compileFlags = 0;

	if (compile_shader("shaders/shadow_mapping.hlsl", nullptr, "shadow_map_vs", "vs_5_0", compileFlags, &shadowMapVS) &&
		compile_shader("shaders/shadow_mapping.hlsl", nullptr, "evsm2_ps", "ps_5_0", compileFlags, &evsm2PS) &&
		reflect_input_layout(shadowMapVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = m_configuration.cullMode;
		psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets         = 1;
		psoDesc.RTVFormats[0]            = m_configuration.evsm2Format;
		psoDesc.DSVFormat                = m_configuration.dsvFormat;
		psoDesc.VS                       = { FUSE_BLOB_ARGS(shadowMapVS) };
		psoDesc.PS                       = { FUSE_BLOB_ARGS(evsm2PS) };
		psoDesc.InputLayout              = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature           = m_shadowMapRS.get();
		psoDesc.SampleMask               = UINT_MAX;
		psoDesc.SampleDesc               = { 1, 0 };
		psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_evsm2PSO)));

	}

	return false;

}