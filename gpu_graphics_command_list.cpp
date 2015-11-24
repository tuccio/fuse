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

bool gpu_graphics_command_list::init(ID3D12Device * device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator * commandAllocator, ID3D12PipelineState * initialState)
{
	shutdown();
	
	if (!FUSE_HR_FAILED(device->CreateCommandList(nodeMask, type, commandAllocator, initialState, IID_PPV_ARGS(get_address()))))
	{
		m_commandAllocator = commandAllocator;
		return true;
	}

	return false;

}

void gpu_graphics_command_list::shutdown(void)
{
	m_commandAllocator.reset();
	reset();
}

void gpu_graphics_command_list::reset_command_list(ID3D12PipelineState * state)
{
	FUSE_HR_CHECK(get()->Reset(m_commandAllocator.get(), state));
}

void gpu_graphics_command_list::reset_command_allocator(void)
{
	FUSE_HR_CHECK(m_commandAllocator->Reset());
}