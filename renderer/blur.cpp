#include "blur.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

using namespace fuse;

bool box_blur::init(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height)
{
	return create_pso(device, type, kernelSize, width, height);
}

void box_blur::shutdown(void)
{
	m_horizontalPSO.reset();
	m_verticalPSO.reset();
}

void box_blur::render(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	ID3D12DescriptorHeap * descriptorHeap,
	ID3D12Resource * const * buffers,
	const D3D12_GPU_DESCRIPTOR_HANDLE * srvs,
	const D3D12_GPU_DESCRIPTOR_HANDLE * uavs)
{

	commandList.reset_command_list(m_horizontalPSO.get());

	commandList.resource_barrier_transition(buffers[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.resource_barrier_transition(buffers[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetDescriptorHeaps(1, &descriptorHeap);

	commandList->SetComputeRootSignature(m_rs.get());
	commandList->SetComputeRootDescriptorTable(0, srvs[0]);
	commandList->SetComputeRootDescriptorTable(1, uavs[1]);

	commandList->Dispatch(m_gridWidth, m_gridHeight, 1);

	FUSE_HR_CHECK(commandList->Close());

	commandQueue.execute(commandList);

	commandList.reset_command_list(m_verticalPSO.get());

	commandList.resource_barrier_transition(buffers[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList.resource_barrier_transition(buffers[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandList->SetDescriptorHeaps(1, &descriptorHeap);

	commandList->SetComputeRootSignature(m_rs.get());
	commandList->SetComputeRootDescriptorTable(0, srvs[1]);
	commandList->SetComputeRootDescriptorTable(1, uavs[0]);

	commandList->Dispatch(m_gridWidth, m_gridHeight, 1);

	FUSE_HR_CHECK(commandList->Close());

	commandQueue.execute(commandList);

}

bool box_blur::create_pso(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height)
{

	com_ptr<ID3DBlob> horizontalCS, verticalCS;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE srvDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE uavDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &srvDescRange);
	rootParameters[1].InitAsDescriptorTable(1, &uavDescRange);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	std::string widthStr      = std::to_string(width);
	std::string heightStr     = std::to_string(height);
	std::string kernelSizeStr = std::to_string(kernelSize);

	D3D_SHADER_MACRO horizontalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "HORIZONTAL", "" },
		{ "TYPE", type },
		{ nullptr, nullptr }
	};

	D3D_SHADER_MACRO verticalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "VERTICAL", "" },
		{ "TYPE", type },
		{ nullptr, nullptr }
	};

	if (compile_shader("shaders/box_blur.hlsl", horizontalDefines, "box_blur_cs", "cs_5_0", 0, &horizontalCS) &&
	    compile_shader("shaders/box_blur.hlsl", verticalDefines, "box_blur_cs", "cs_5_0", 0, &verticalCS) &&
	    !FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
	    !FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_COMPUTE_PIPELINE_STATE_DESC horizontalPSODesc = {};

		horizontalPSODesc.CS = { FUSE_BLOB_ARGS(horizontalCS) };
		horizontalPSODesc.pRootSignature = m_rs.get();

		D3D12_COMPUTE_PIPELINE_STATE_DESC verticalPSODesc = {};

		verticalPSODesc.CS = { FUSE_BLOB_ARGS(verticalCS) };
		verticalPSODesc.pRootSignature = m_rs.get();

		const UINT groupSize = 32;

		m_gridWidth = (width + groupSize - 1) / groupSize;
		m_gridHeight = (height + groupSize - 1) / groupSize;

		return !FUSE_HR_FAILED(device->CreateComputePipelineState(&horizontalPSODesc, IID_PPV_ARGS(&m_horizontalPSO))) &&
		       !FUSE_HR_FAILED(device->CreateComputePipelineState(&verticalPSODesc, IID_PPV_ARGS(&m_verticalPSO)));

	}

	return false;

}