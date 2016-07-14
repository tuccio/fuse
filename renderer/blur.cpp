#include "blur.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/math.hpp>

#include <fuse/descriptor_heap.hpp>

#include <sstream>
#include <algorithm>

using namespace fuse;

bool blur::init_box_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height)
{
	return create_box_pso(device, type, kernelSize, width, height);
}

void blur::shutdown(void)
{
	m_horizontalPSO.reset();
	m_verticalPSO.reset();
}

void blur::render(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	const render_resource & source,
	const render_resource & pingPongBuffer)
{

	commandList->SetPipelineState(m_horizontalPSO.get());

	commandList.resource_barrier_transition(source.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.resource_barrier_transition(pingPongBuffer.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetComputeRootSignature(m_rs.get());
	commandList->SetComputeRootDescriptorTable(0, source.get_srv_gpu_descriptor_handle());
	commandList->SetComputeRootDescriptorTable(1, pingPongBuffer.get_uav_gpu_descriptor_handle());

	commandList->Dispatch(m_gridWidth, m_gridHeight, 1);

	commandList->SetPipelineState(m_verticalPSO.get());

	commandList.resource_barrier_transition(source.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList.resource_barrier_transition(pingPongBuffer.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandList->SetComputeRootSignature(m_rs.get());
	commandList->SetComputeRootDescriptorTable(0, pingPongBuffer.get_srv_gpu_descriptor_handle());
	commandList->SetComputeRootDescriptorTable(1, source.get_uav_gpu_descriptor_handle());

	commandList->Dispatch(m_gridWidth, m_gridHeight, 1);

}

bool blur::create_box_pso(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height)
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

	if (compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), horizontalDefines, "box_blur_cs", "cs_5_0", 0, &horizontalCS) &&
	    compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), verticalDefines, "box_blur_cs", "cs_5_0", 0, &verticalCS) &&
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

static float gaussian_weight(float sigma, float distance)
{

	float s2 = sigma * sigma;
	float G  = 1.f / std::sqrt(FUSE_TWO_PI * s2);

	return G * std::exp(-.5f * (distance * distance) / s2);

}

static std::string make_gaussian_kernel(float sigma, uint32_t kernelSize)
{

	std::ostringstream gaussKernelSS;

	gaussKernelSS.precision(10);
	gaussKernelSS.setf(std::ios::fixed, std::ios::floatfield);

	std::vector<float> weights(kernelSize);

	int kernelEnd = kernelSize >> 1;
	int kernelStart = -kernelEnd;

	for (int i = kernelStart; i <= kernelEnd; i++)
	{
		weights[i + kernelEnd] = gaussian_weight(sigma, i);
	}

	std::copy(weights.begin(), weights.end() - 1, std::ostream_iterator<float>(gaussKernelSS, ", "));
	gaussKernelSS << weights.back();

	return gaussKernelSS.str();

}

bool blur::init_gaussian_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, float sigma, uint32_t width, uint32_t height)
{

	if (kernelSize < 3)
	{
		return false;
	}

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

	std::string gaussKernel = make_gaussian_kernel(sigma, kernelSize);

	D3D_SHADER_MACRO horizontalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "HORIZONTAL", "" },
		{ "TYPE", type },
		{ "GAUSS_KERNEL", gaussKernel.c_str() },
		{ nullptr, nullptr }
	};

	D3D_SHADER_MACRO verticalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "VERTICAL", "" },
		{ "TYPE", type },
		{ "GAUSS_KERNEL", gaussKernel.c_str() },
		{ nullptr, nullptr }
	};

	if (compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), horizontalDefines, "gaussian_blur_cs", "cs_5_0", 0, &horizontalCS) &&
	    compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), verticalDefines, "gaussian_blur_cs", "cs_5_0", 0, &verticalCS) &&
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

bool blur::init_bilateral_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, float sigma, uint32_t width, uint32_t height)
{

	if (kernelSize < 3)
	{
		return false;
	}

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

	std::string widthStr = std::to_string(width);
	std::string heightStr = std::to_string(height);
	std::string kernelSizeStr = std::to_string(kernelSize);

	std::string gaussKernel = make_gaussian_kernel(sigma, kernelSize);

	D3D_SHADER_MACRO horizontalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "HORIZONTAL", "" },
		{ "TYPE", type },
		{ "GAUSS_KERNEL", gaussKernel.c_str() },
		{ nullptr, nullptr }
	};

	D3D_SHADER_MACRO verticalDefines[] = {
		{ "WIDTH", widthStr.c_str() },
		{ "HEIGHT", heightStr.c_str() },
		{ "KERNEL_SIZE", kernelSizeStr.c_str() },
		{ "VERTICAL", "" },
		{ "TYPE", type },
		{ "GAUSS_KERNEL", gaussKernel.c_str() },
		{ nullptr, nullptr }
	};

	if (compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), horizontalDefines, "bilateral_blur_cs", "cs_5_0", 0, &horizontalCS) &&
		compile_shader(FUSE_LITERAL("shaders/blur.hlsl"), verticalDefines, "bilateral_blur_cs", "cs_5_0", 0, &verticalCS) &&
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