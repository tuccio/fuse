#pragma once

#include <fuse/directx_helper.hpp>

#include <unordered_map>

namespace fuse
{

	class gpu_graphics_command_list :
		public com_ptr<ID3D12GraphicsCommandList>
	{

	public:

		inline const gpu_graphics_command_list * operator& (void) const { return this; }
		inline gpu_graphics_command_list * operator&(void) { return this; }

		void resource_barrier_transition(ID3D12Resource * resource, D3D12_RESOURCE_STATES requestedState);

	private:

		friend class gpu_command_queue;

		std::unordered_map<ID3D12Resource*, std::pair<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES>> m_pending;

		

	};

}