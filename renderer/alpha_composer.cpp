#include "alpha_composer.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/descriptor_heap.hpp>

using namespace fuse;

bool alpha_composer::init(ID3D12Device * device, const alpha_composer_configuration & cfg)
{
	m_configuration = cfg;
	return create_psos(device);
}

void alpha_composer::shutdown(void)
{
	m_pso.reset();
	m_rs.reset();
}

void alpha_composer::render(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	const render_resource & renderTarget,
	const render_resource * const * shaderResources,
	uint32_t numSRVs)
{

	commandList->SetPipelineState(m_pso.get());

	commandList->OMSetRenderTargets(1, &renderTarget.get_rtv_cpu_descriptor_handle(), false, nullptr);

	commandList.resource_barrier_transition(renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	for (uint32_t i = 0; i < numSRVs; i++)
	{

		commandList.resource_barrier_transition(shaderResources[i]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		commandList->SetGraphicsRootSignature(m_rs.get());
		commandList->SetGraphicsRootDescriptorTable(0, shaderResources[i]->get_srv_gpu_descriptor_handle());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);

	}

}

bool alpha_composer::create_psos(ID3D12Device * device)
{

	com_ptr<ID3DBlob> quadVS, composerPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[1];

	CD3DX12_DESCRIPTOR_RANGE srv;

	srv.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &srv, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT),
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (compile_shader("shaders/quad_vs.hlsl", nullptr, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
		compile_shader("shaders/composer.hlsl", nullptr, "composer_ps", "ps_5_0", compileFlags, &composerPS) &&
		reflect_input_layout(quadVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

		psoDesc.BlendState                             = m_configuration.blendDesc;
		psoDesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState                      = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable          = FALSE;
		psoDesc.NumRenderTargets                       = 1;
		psoDesc.RTVFormats[0]                          = m_configuration.rtvFormat;
		psoDesc.VS                                     = { FUSE_BLOB_ARGS(quadVS) };
		psoDesc.PS                                     = { FUSE_BLOB_ARGS(composerPS) };
		psoDesc.InputLayout                            = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                         = m_rs.get();
		psoDesc.SampleMask                             = UINT_MAX;
		psoDesc.SampleDesc                             = { 1, 0 };
		psoDesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	}

	return false;

}