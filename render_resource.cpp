#include <fuse/render_resource.hpp>
#include <fuse/descriptor_heap.hpp>
#include <fuse/gpu_global_resource_state.hpp>

using namespace fuse;

render_resource::render_resource(void) :
	m_rtvToken(FUSE_POOL_INVALID),
	m_uavToken(FUSE_POOL_INVALID),
	m_srvToken(FUSE_POOL_INVALID),
	m_dsvToken(FUSE_POOL_INVALID) { }

render_resource::~render_resource(void) { }

bool render_resource::create(ID3D12Device * device, const D3D12_RESOURCE_DESC * desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE * clearValue)
{

	return gpu_global_resource_state::get_singleton_pointer()->
		create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			desc,
			initialState,
			clearValue,
			IID_PPV_ARGS(get_address()));

}

void render_resource::clear(void)
{

	rtv_descriptor_heap::get_singleton_pointer()->free(m_rtvToken);
	cbv_uav_srv_descriptor_heap::get_singleton_pointer()->free(m_uavToken);
	cbv_uav_srv_descriptor_heap::get_singleton_pointer()->free(m_srvToken);
	dsv_descriptor_heap::get_singleton_pointer()->free(m_dsvToken);

	m_rtvToken = FUSE_POOL_INVALID;
	m_uavToken = FUSE_POOL_INVALID;
	m_srvToken = FUSE_POOL_INVALID;
	m_dsvToken = FUSE_POOL_INVALID;

}

/* Views */

bool render_resource::create_render_target_view(ID3D12Device * device, const D3D12_RENDER_TARGET_VIEW_DESC * desc)
{

	if (m_rtvToken == FUSE_POOL_INVALID)
	{
		m_rtvToken = rtv_descriptor_heap::get_singleton_pointer()->create_render_target_view(device, get(), desc);
	}
	else
	{
		device->CreateRenderTargetView(get(), desc, rtv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_rtvToken));
	}

	return m_rtvToken != FUSE_POOL_INVALID;

}

bool render_resource::create_unordered_access_view(ID3D12Device * device, const D3D12_UNORDERED_ACCESS_VIEW_DESC * desc)
{

	if (m_uavToken == FUSE_POOL_INVALID)
	{
		m_uavToken = cbv_uav_srv_descriptor_heap::get_singleton_pointer()->create_unordered_access_view(device, get(), nullptr, desc);
	}
	else
	{
		device->CreateUnorderedAccessView(get(), nullptr, desc, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_uavToken));
	}

	return m_uavToken != FUSE_POOL_INVALID;

}

bool render_resource::create_shader_resource_view(ID3D12Device * device, const D3D12_SHADER_RESOURCE_VIEW_DESC * desc)
{

	if (m_srvToken == FUSE_POOL_INVALID)
	{
		m_srvToken = cbv_uav_srv_descriptor_heap::get_singleton_pointer()->create_shader_resource_view(device, get(), desc);
	}
	else
	{
		device->CreateShaderResourceView(get(), desc, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_srvToken));
	}

	return m_srvToken != FUSE_POOL_INVALID;

}

bool render_resource::create_depth_stencil_view(ID3D12Device * device, const D3D12_DEPTH_STENCIL_VIEW_DESC * desc)
{

	if (m_dsvToken == FUSE_POOL_INVALID)
	{
		m_dsvToken = dsv_descriptor_heap::get_singleton_pointer()->create_depth_stencil_view(device, get(), desc);
	}
	else
	{
		device->CreateDepthStencilView(get(), desc, dsv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_dsvToken));
	}

	return m_dsvToken != FUSE_POOL_INVALID;

}

/* Views getters */

D3D12_CPU_DESCRIPTOR_HANDLE render_resource::get_rtv_cpu_descriptor_handle(void) const
{
	return rtv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_rtvToken);
}

D3D12_GPU_DESCRIPTOR_HANDLE render_resource::get_rtv_gpu_descriptor_handle(void) const
{
	return rtv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(m_rtvToken);
}

D3D12_CPU_DESCRIPTOR_HANDLE render_resource::get_uav_cpu_descriptor_handle(void) const
{
	return cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_uavToken);
}

D3D12_GPU_DESCRIPTOR_HANDLE render_resource::get_uav_gpu_descriptor_handle(void) const
{
	return cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(m_uavToken);
}

D3D12_CPU_DESCRIPTOR_HANDLE render_resource::get_srv_cpu_descriptor_handle(void) const
{
	return cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_srvToken);
}

D3D12_GPU_DESCRIPTOR_HANDLE render_resource::get_srv_gpu_descriptor_handle(void) const
{
	return cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(m_srvToken);
}

D3D12_CPU_DESCRIPTOR_HANDLE render_resource::get_dsv_cpu_descriptor_handle(void) const
{
	return dsv_descriptor_heap::get_singleton_pointer()->get_cpu_descriptor_handle(m_dsvToken);
}

D3D12_GPU_DESCRIPTOR_HANDLE render_resource::get_dsv_gpu_descriptor_handle(void) const
{
	return dsv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(m_dsvToken);
}