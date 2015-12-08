#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_global_resource_state.hpp>

#include <d3dx12.h>

using namespace fuse;

gpu_command_queue::gpu_command_queue(void)
{
	m_hFenceEvent = NULL;
}

gpu_command_queue::~gpu_command_queue(void)
{
	shutdown();
}

bool gpu_command_queue::init(ID3D12Device * device, const D3D12_COMMAND_QUEUE_DESC * queueDesc)
{

	D3D12_COMMAND_QUEUE_DESC defaultQueueDesc = {};

	queueDesc = queueDesc ? queueDesc : &defaultQueueDesc;

	m_hFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

	if (m_hFenceEvent &&
	    !FUSE_HR_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))) &&
		!FUSE_HR_FAILED(device->CreateCommandQueue(queueDesc, IID_PPV_ARGS(get_address()))))
	{
		
		m_frameIndex = 0;
		m_fenceValue = 0;

		return true;

	}

	shutdown();

	return false;

}

void gpu_command_queue::shutdown(void)
{
	
	if (m_hFenceEvent)
	{
		
		if (get())
		{
			advance_frame_index();
			wait_for_frame(m_frameIndex);
		}

		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = NULL;

	}

	if (m_fence)
	{
		m_fence.reset();
	}

	reset();

}

bool gpu_command_queue::wait_for_frame(UINT64 fenceValue, UINT time) const
{

	collect_garbage();

	m_fenceValue = m_fence->GetCompletedValue();

	if (m_fenceValue >= fenceValue)
	{
		return true;
	}

	FUSE_HR_CHECK(m_fence->SetEventOnCompletion(fenceValue, m_hFenceEvent));

	if (WaitForSingleObject(m_hFenceEvent, time) == WAIT_OBJECT_0)
	{
		m_fenceValue = fenceValue;
		return true;
	}

	return false;

}

void gpu_command_queue::execute(gpu_graphics_command_list & commandList)
{

	if (commandList.m_pending.empty())
	{
		get()->ExecuteCommandLists(1, (ID3D12CommandList**)commandList.get_address());
	}
	else
	{

		std::vector<D3D12_RESOURCE_BARRIER> barriers;

		barriers.reserve(commandList.m_pending.size());

		auto globalStateManager = gpu_global_resource_state::get_singleton_pointer();

		for (auto & p : commandList.m_pending)
		{

			ID3D12Resource * resource = p.first;
			
			D3D12_RESOURCE_STATES desiredState = p.second.first;
			D3D12_RESOURCE_STATES globalState = globalStateManager->get_state(resource);

			if (globalState != desiredState)
			{
				barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, globalState, desiredState));
			}

		}

		if (barriers.empty())
		{
			get()->ExecuteCommandLists(1, (ID3D12CommandList**)commandList.get_address());
		}
		else
		{
			
			//FUSE_HR_CHECK(m_auxCommandList->Reset(m_auxCommandAllocator, nullptr));
			m_auxCommandList->reset_command_list(nullptr);
			(*m_auxCommandList)->ResourceBarrier(barriers.size(), &barriers[0]);

			ID3D12CommandList * lists[2] = { m_auxCommandList->get(), commandList.get() };

			FUSE_HR_CHECK((*m_auxCommandList)->Close());

			get()->ExecuteCommandLists(2, lists);

			for (auto & barrier : barriers)
			{
				globalStateManager->set_state(barrier.Transition.pResource, barrier.Transition.StateAfter);
			}

		}

	}

	commandList.m_pending.clear();

}

void gpu_command_queue::safe_release(IUnknown * resource) const
{
	m_garbage.push(garbage_type(m_frameIndex, com_ptr<IUnknown>(resource)));
}

void gpu_command_queue::collect_garbage(void) const
{
	while (!m_garbage.empty() && m_garbage.front().first < m_fenceValue)
	{
		m_garbage.pop();
	}
}

void gpu_command_queue::set_aux_command_list(gpu_graphics_command_list & commandList)
{
	m_auxCommandList = std::addressof(commandList);
}