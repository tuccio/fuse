#include <fuse/gpu_upload_manager.hpp>

#include <d3dx12.h>

using namespace fuse;

gpu_upload_manager::gpu_upload_manager(ID3D12Device * device, ID3D12CommandQueue * commandQueue) :
	m_commandQueue(commandQueue),
	m_lastAllocated(0)
{

	FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	FUSE_HR_CHECK(m_commandAllocator->SetName(L"fuse_upload_manager_command_allocator"));
	FUSE_HR_CHECK(m_commandList->SetName(L"fuse_upload_manager_command_list"));

	FUSE_HR_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	FUSE_HR_CHECK(m_fence->SetName(L"fuse_upload_manager_fence"));

	FUSE_HR_CHECK(m_commandList->Close());

}

UINT64 gpu_upload_manager::upload(ID3D12Device * device,
	                              ID3D12Resource * dest,
	                              UINT firstSubresource,
	                              UINT numSubresources,
	                              D3D12_SUBRESOURCE_DATA * data,
                                  D3D12_RESOURCE_BARRIER * before,
                                  D3D12_RESOURCE_BARRIER * after)
{

	com_ptr<ID3D12Resource> heap;

	UINT64 id = 0;
	
	if (!FUSE_HR_FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&dest->GetDesc(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&heap))) &&
		!FUSE_HR_FAILED(m_commandList->Reset(m_commandAllocator.get(), nullptr)))
	{

		id = ++m_lastAllocated;

		FUSE_HR_CHECK(heap->SetName(L"fuse_upload_manager_heap"));

		if (before)
		{
			m_commandList->ResourceBarrier(1, before);
		}

		UpdateSubresources(m_commandList.get(), dest, heap.get(), 0, firstSubresource, numSubresources, data);

		if (after)
		{
			m_commandList->ResourceBarrier(1, after);
		}

		m_commandList->Close();

		ID3D12CommandList * commandLists[] = { m_commandList.get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);

		m_commandQueue->Signal(m_fence.get(), id);

		m_heaps.emplace_hint(m_heaps.rbegin().base(), id, std::move(heap));

	}

	return id;

}

gpu_upload_manager::~gpu_upload_manager(void)
{
	wait();
}

void gpu_upload_manager::collect_garbage(void)
{
	
	UINT64 lastCompleted = m_fence->GetCompletedValue();

	// Erase all objects up until last completed

	auto beginIt = m_heaps.begin();
	auto endIt   = m_heaps.upper_bound(lastCompleted);

	if (beginIt != endIt)
	{
		m_heaps.erase(beginIt, endIt);
	}
	
	if (m_heaps.empty())
	{
		FUSE_HR_CHECK(m_commandAllocator->Reset());
	}
	
}

bool gpu_upload_manager::wait(UINT waitTime)
{

	if (m_heaps.empty())
	{
		return true;
	}

	// Wait for the last added object to be finished

	UINT64 maxFenceValue = m_heaps.rbegin()->first;
	UINT   waitResult    = 0;

	if (m_fence->GetCompletedValue() < maxFenceValue)
	{

		HANDLE hEventUpload = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		m_fence->SetEventOnCompletion(maxFenceValue, hEventUpload);
		
		waitResult = WaitForSingleObject(hEventUpload, waitTime);

		CloseHandle(hEventUpload);

	}

	bool success = !waitResult;

	if (success)
	{
		m_heaps.clear();
		FUSE_HR_CHECK(m_commandAllocator->Reset());
	}

	return success;

}

bool gpu_upload_manager::wait_for_id(UINT64 id, UINT waitTime)
{

	UINT waitResult = 0;

	if (m_fence->GetCompletedValue() < id)
	{

		HANDLE hEventUpload = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		m_fence->SetEventOnCompletion(id, hEventUpload);

		waitResult = WaitForSingleObject(hEventUpload, waitTime);

		CloseHandle(hEventUpload);

	}

	bool success = !waitResult;

	if (success)
	{
		collect_garbage();
	}

	return success;

}