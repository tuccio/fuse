#include <fuse/gpu_ring_buffer.hpp>

#include <d3dx12.h>

using namespace fuse;

bool gpu_ring_buffer::create(ID3D12Device * device, const D3D12_HEAP_DESC * desc)
{

	if (!FUSE_HR_FAILED(device->CreateHeap(desc, IID_PPV_ARGS(&m_heap))))
	{

		m_heap->SetName(L"gpu_ring_buffer_heap");

		m_size       = desc->SizeInBytes;
		m_alignment  = desc->Alignment;
		m_offset     = 0;
		m_freeSpace  = m_size;

		return true;

	}

	return false;

}

bool gpu_ring_buffer::allocate(ID3D12Device * device,
                               gpu_command_queue & commandQueue,
                               ID3D12Resource ** resource,
                               const D3D12_RESOURCE_DESC * desc,
                               D3D12_RESOURCE_STATES initialState,
                               const D3D12_CLEAR_VALUE * optimizedClearValue)
{

	assert(desc->Alignment == m_alignment && "Resource and heap allocation mismatch, the resource cannot be allocated on this heap.");

	D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, desc);

	UINT allocationSize = allocationInfo.SizeInBytes;

	if (allocationSize > m_size)
	{
		FUSE_LOG_OPT("gpu_ring_buffer", "Not enough space in the ring buffer for the requested resource.");
		return false;
	}

	while (m_freeSpace < allocationSize)
	{

		if (m_allocatedChunks.empty())
		{
			m_offset = 0;
			m_freeSpace = m_size;
		}
		else
		{

			auto & chunk = m_allocatedChunks.front();
			commandQueue.wait_for_frame(chunk.frameIndex);

			if (chunk.offset == 0)
			{
				m_offset = 0;
				m_freeSpace = chunk.size;
			}
			else
			{
				m_freeSpace += chunk.size;
			}

			m_allocatedChunks.pop();

		}

	}

	assert((m_freeSpace >= allocationSize) && "Not enough space freed for the buffer");

	UINT offset = m_offset;

	m_offset    += allocationSize;
	m_freeSpace -= allocationSize;

	m_allocatedChunks.push(allocated_chunk{ offset, allocationSize, commandQueue.get_frame_index() });

	return !FUSE_HR_FAILED(device->CreatePlacedResource(
		m_heap.get(),
		offset,
		desc,
		initialState,
		optimizedClearValue,
		IID_PPV_ARGS(resource)));

}