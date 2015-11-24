#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/properties_macros.hpp>

#include <unordered_map>

namespace fuse
{

	class gpu_graphics_command_list :
		public com_ptr<ID3D12GraphicsCommandList>
	{

	public:

		bool init(ID3D12Device * device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator * commandAllocator, ID3D12PipelineState * initialState);
		void shutdown(void);

		void reset_command_list(ID3D12PipelineState * state);
		void reset_command_allocator(void);

		inline const gpu_graphics_command_list * operator& (void) const { return this; }
		inline gpu_graphics_command_list * operator&(void) { return this; }

		void resource_barrier_transition(ID3D12Resource * resource, D3D12_RESOURCE_STATES requestedState);

	private:

		friend class gpu_command_queue;

		std::unordered_map<ID3D12Resource*, std::pair<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES>> m_pending;

		com_ptr<ID3D12CommandAllocator> m_commandAllocator;

	public:

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(command_allocator, m_commandAllocator)
		)

	};

}