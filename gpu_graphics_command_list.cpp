#include <fuse/gpu_graphics_command_list.hpp>

using namespace fuse;

void gpu_graphics_command_list::resource_barrier_transition(ID3D12Resource * resource, D3D12_RESOURCE_STATES requestedState)
{

	auto it = m_pending.find(resource);

	if (it != m_pending.end())
	{

		if (it->second.second != requestedState)
		{
			get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, it->second.second, requestedState));
			it->second.second = requestedState;
		}
		
	}
	else
	{
		m_pending[resource] = std::make_pair(requestedState, requestedState);
	}

}