#include "sdsm.hpp"

#include <fuse/gpu_upload_manager.hpp>
#include <fuse/gpu_global_resource_state.hpp>

using namespace fuse;

sdsm::sdsm(void)
{
}

bool sdsm::init(ID3D12Device * device, uint32_t width, uint32_t height)
{

	shutdown();

	m_width  = width;
	m_height = height;

	return create_psos(device);

}

void sdsm::shutdown(void)
{
	m_pingPongBuffer.clear();	
}

void sdsm::create_log_partitions(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, const render_resource & depthBuffer, const render_resource & output)
{
	
	commandList->SetPipelineState(m_minMaxReductionPSO.get());
	commandList->SetComputeRootSignature(m_minMaxReductionRS.get());	

	commandList->SetComputeRootDescriptorTable(1, output.get_uav_gpu_descriptor_handle());

	commandList->Dispatch(1, 1, 1);

}

bool sdsm::create_psos(ID3D12Device * device)
{

	com_ptr<ID3DBlob> minmaxCS;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE srvDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE uavDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &srvDescRange);
	rootParameters[1].InitAsDescriptorTable(1, &uavDescRange);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	if (compile_shader("shaders/sdsm.hlsl", nullptr, "min_max_reduction_cs", "cs_5_0", 0, &minmaxCS) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_minMaxReductionRS))))
	{

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.CS = { FUSE_BLOB_ARGS(minmaxCS) };
		psoDesc.pRootSignature = m_minMaxReductionRS.get();

		return !FUSE_HR_FAILED(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_minMaxReductionPSO)));

	}

	return false;

}

size_t sdsm::get_constant_buffer_size(void)
{
	return sizeof(float) * 4;
}