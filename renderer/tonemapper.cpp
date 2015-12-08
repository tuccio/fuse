#include "tonemapper.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/descriptor_heap.hpp>

using namespace fuse;

bool tonemapper::init(ID3D12Device * device)
{
	return create_pso(device);
}

void tonemapper::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	const render_resource & source,
	const render_resource & destination,
	UINT width,
	UINT height)
{
	
	commandList->SetPipelineState(m_pso.get());

	commandList.resource_barrier_transition(source.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.resource_barrier_transition(destination.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());

	commandList->SetComputeRootSignature(m_rs.get());
	commandList->SetComputeRootDescriptorTable(0, source.get_srv_gpu_descriptor_handle());
	commandList->SetComputeRootDescriptorTable(1, destination.get_uav_gpu_descriptor_handle());

	UINT groupSize = 32;

	UINT gridWidth  = (width + groupSize - 1) / groupSize;
	UINT gridHeight = (height + groupSize - 1) / groupSize;

	commandList->Dispatch(gridWidth, gridHeight, 1);

}

bool tonemapper::create_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> tonemapCS;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE srvDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE uavDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &srvDescRange);
	rootParameters[1].InitAsDescriptorTable(1, &uavDescRange);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	if (compile_shader("shaders/tonemapping.hlsl", nullptr, "tonemap_cs", "cs_5_0", 0, &tonemapCS) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.CS = { FUSE_BLOB_ARGS(tonemapCS) };
		psoDesc.pRootSignature = m_rs.get();

		return !FUSE_HR_FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	}

	return false;

}