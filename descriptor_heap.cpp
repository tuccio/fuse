#include <fuse/descriptor_heap.hpp>

using namespace fuse;

bool descriptor_heap::init(ID3D12Device * device, const D3D12_DESCRIPTOR_HEAP_DESC * desc)
{
	
	shutdown();

	if (!FUSE_HR_FAILED(device->CreateDescriptorHeap(desc, IID_PPV_ARGS(get_address()))))
	{
		
		m_maxDescriptors = desc->NumDescriptors;
		m_poolManager    = std::move(pool_manager(m_maxDescriptors));

		m_cpuHeapHandle  = get()->GetCPUDescriptorHandleForHeapStart();
		m_gpuHeapHandle  = get()->GetGPUDescriptorHandleForHeapStart();

		m_descriptorSize = device->GetDescriptorHandleIncrementSize(desc->Type);

		return true;

	}

	return false;

}

void descriptor_heap::shutdown(void)
{
	m_maxDescriptors = 0;
	reset();
}

descriptor_token_t descriptor_heap::allocate(UINT descriptors)
{
	return m_poolManager.allocate(descriptors);
}

void descriptor_heap::free(UINT element, UINT descriptors)
{
	m_poolManager.free(element, descriptors);
}

D3D12_CPU_DESCRIPTOR_HANDLE descriptor_heap::get_cpu_descriptor_handle(UINT token)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cpuHeapHandle, token, m_descriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE descriptor_heap::get_gpu_descriptor_handle(UINT token)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_gpuHeapHandle, token, m_descriptorSize);
}

/* SRV/UAV/CBV */

bool cbv_uav_srv_descriptor_heap::init(ID3D12Device * device, UINT maxDescriptors)
{

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

	descriptorHeapDesc.NumDescriptors = maxDescriptors;
	descriptorHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	return descriptor_heap::init(device, &descriptorHeapDesc);

}

void cbv_uav_srv_descriptor_heap::shutdown(void)
{
	descriptor_heap::shutdown();
}

descriptor_token_t cbv_uav_srv_descriptor_heap::create_shader_resource_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_SHADER_RESOURCE_VIEW_DESC * desc)
{

	auto token = allocate();

	if (token != FUSE_POOL_INVALID)
	{
		device->CreateShaderResourceView(resource, desc, get_cpu_descriptor_handle(token));
	}

	return token;

}

descriptor_token_t cbv_uav_srv_descriptor_heap::create_unordered_access_view(ID3D12Device * device, ID3D12Resource * resource, ID3D12Resource * counter, const D3D12_UNORDERED_ACCESS_VIEW_DESC * desc)
{

	auto token = allocate();

	if (token != FUSE_POOL_INVALID)
	{
		device->CreateUnorderedAccessView(resource, counter, desc, get_cpu_descriptor_handle(token));
	}

	return token;

}

descriptor_token_t cbv_uav_srv_descriptor_heap::create_constant_buffer_view(ID3D12Device * device, const D3D12_CONSTANT_BUFFER_VIEW_DESC * desc)
{

	auto token = allocate();

	if (token != FUSE_POOL_INVALID)
	{
		device->CreateConstantBufferView(desc, get_cpu_descriptor_handle(token));
	}

	return token;

}

void cbv_uav_srv_descriptor_heap::free(UINT token)
{
	descriptor_heap::free(token);
}

/* RTV */

bool rtv_descriptor_heap::init(ID3D12Device * device, UINT maxDescriptors)
{

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

	descriptorHeapDesc.NumDescriptors = maxDescriptors;
	descriptorHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	return descriptor_heap::init(device, &descriptorHeapDesc);

}

void rtv_descriptor_heap::shutdown(void)
{
	descriptor_heap::shutdown();
}

descriptor_token_t rtv_descriptor_heap::create_render_target_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_RENDER_TARGET_VIEW_DESC * desc)
{

	auto token = allocate();

	if (token != FUSE_POOL_INVALID)
	{
		device->CreateRenderTargetView(resource, desc, get_cpu_descriptor_handle(token));
	}

	return token;

}

void rtv_descriptor_heap::free(UINT token)
{
	descriptor_heap::free(token);
}

/* DSV */

bool dsv_descriptor_heap::init(ID3D12Device * device, UINT maxDescriptors)
{

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

	descriptorHeapDesc.NumDescriptors = maxDescriptors;
	descriptorHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	return descriptor_heap::init(device, &descriptorHeapDesc);

}

void dsv_descriptor_heap::shutdown(void)
{
	descriptor_heap::shutdown();
}

descriptor_token_t dsv_descriptor_heap::create_depth_stencil_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_DEPTH_STENCIL_VIEW_DESC * desc)
{

	auto token = allocate();

	if (token != FUSE_POOL_INVALID)
	{
		device->CreateDepthStencilView(resource, desc, get_cpu_descriptor_handle(token));
	}

	return token;

}

void dsv_descriptor_heap::free(UINT token)
{
	descriptor_heap::free(token);
}