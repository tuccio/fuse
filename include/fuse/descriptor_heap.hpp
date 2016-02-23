#pragma once

#include <fuse/singleton.hpp>
#include <fuse/directx_helper.hpp>

#include <fuse/properties_macros.hpp>
#include <fuse/pool_manager.hpp>

#define FUSE_DESCRIPTOR_INVALID (FUSE_POOL_INVALID)

namespace fuse
{

	typedef pool_size_t descriptor_token_t;

	class descriptor_heap :
		public com_ptr<ID3D12DescriptorHeap>
	{

	public:

		D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_descriptor_handle(UINT token);
		D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_descriptor_handle(UINT token);

		descriptor_token_t allocate(UINT descriptors = 1);
		void free(UINT element, UINT descriptors = 1);

	protected:

		UINT m_maxDescriptors;
		UINT m_descriptorSize;

		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHeapHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHeapHandle;

		bool init(ID3D12Device * device, const D3D12_DESCRIPTOR_HEAP_DESC * desc);
		void shutdown(void);

	private:

		pool_manager m_poolManager;

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(max_descriptors, m_maxDescriptors)
			(descriptor_size, m_descriptorSize)
		)

	};

	/* Heaps */

	class cbv_uav_srv_descriptor_heap :
		public singleton<cbv_uav_srv_descriptor_heap>,
		public descriptor_heap
	{

	public:

		bool init(ID3D12Device * device, UINT maxDescriptors);
		void shutdown(void);

		descriptor_token_t create_shader_resource_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_SHADER_RESOURCE_VIEW_DESC * desc = nullptr);
		descriptor_token_t create_unordered_access_view(ID3D12Device * device, ID3D12Resource * resource, ID3D12Resource * counter = nullptr, const D3D12_UNORDERED_ACCESS_VIEW_DESC * desc = nullptr);
		descriptor_token_t create_constant_buffer_view(ID3D12Device * device, const D3D12_CONSTANT_BUFFER_VIEW_DESC * desc);

		void free(UINT token);

	};

	class rtv_descriptor_heap :
		public singleton<rtv_descriptor_heap>,
		public descriptor_heap
	{

	public:

		bool init(ID3D12Device * device, UINT maxDescriptors);
		void shutdown(void);

		descriptor_token_t create_render_target_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_RENDER_TARGET_VIEW_DESC * desc = nullptr);

		void free(UINT token);

	};

	class dsv_descriptor_heap :
		public singleton<dsv_descriptor_heap>,
		public descriptor_heap
	{

	public:

		bool init(ID3D12Device * device, UINT maxDescriptors);
		void shutdown(void);

		descriptor_token_t create_depth_stencil_view(ID3D12Device * device, ID3D12Resource * resource, const D3D12_DEPTH_STENCIL_VIEW_DESC * desc = nullptr);

		void free(UINT token);

	};	

}