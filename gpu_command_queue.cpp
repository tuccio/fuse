#include <fuse/gpu_command_queue.hpp>

#include <d3dx12.h>

using namespace fuse;

gpu_command_queue::gpu_command_queue(void)
{
	m_hFenceEvent = NULL;
}

gpu_command_queue::~gpu_command_queue(void)
{
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

	if (update_fence_value() >= fenceValue)
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