#pragma once

#include <fuse/directx_helper.hpp>

#include <fuse/properties_macros.hpp>

namespace fuse
{

	class render_resource :
		public com_ptr<ID3D12Resource>
	{

	public:

		render_resource(void);
		render_resource(const render_resource &s) = delete;
		render_resource(render_resource &&) = default;

		~render_resource(void);

		bool create(ID3D12Device * device, const D3D12_RESOURCE_DESC * desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE * clearValue);
		void clear(void);

		bool create_render_target_view(ID3D12Device * device, const D3D12_RENDER_TARGET_VIEW_DESC * desc = nullptr);
		bool create_unordered_access_view(ID3D12Device * device, const D3D12_UNORDERED_ACCESS_VIEW_DESC * desc = nullptr);
		bool create_shader_resource_view(ID3D12Device * device, const D3D12_SHADER_RESOURCE_VIEW_DESC * desc = nullptr);
		bool create_depth_stencil_view(ID3D12Device * device, const D3D12_DEPTH_STENCIL_VIEW_DESC * desc = nullptr);

		D3D12_CPU_DESCRIPTOR_HANDLE get_rtv_cpu_descriptor_handle(void) const;
		D3D12_GPU_DESCRIPTOR_HANDLE get_rtv_gpu_descriptor_handle(void) const;

		D3D12_CPU_DESCRIPTOR_HANDLE get_uav_cpu_descriptor_handle(void) const;
		D3D12_GPU_DESCRIPTOR_HANDLE get_uav_gpu_descriptor_handle(void) const;

		D3D12_CPU_DESCRIPTOR_HANDLE get_srv_cpu_descriptor_handle(void) const;
		D3D12_GPU_DESCRIPTOR_HANDLE get_srv_gpu_descriptor_handle(void) const;

		D3D12_CPU_DESCRIPTOR_HANDLE get_dsv_cpu_descriptor_handle(void) const;
		D3D12_GPU_DESCRIPTOR_HANDLE get_dsv_gpu_descriptor_handle(void) const;

	private:

		UINT m_rtvToken;
		UINT m_uavToken;
		UINT m_srvToken;
		UINT m_dsvToken;

	public:

		FUSE_PROPERTIES_BY_VALUE (
			(rtv_token, m_rtvToken)
			(uav_token, m_uavToken)
			(srv_token, m_srvToken)
			(dsv_token, m_dsvToken)
		)

	};

}