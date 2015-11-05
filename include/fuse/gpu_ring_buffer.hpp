#pragma once

#include <d3d12.h>

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>

#include <queue>

namespace fuse
{

	class gpu_ring_buffer
	{

	public:

		gpu_ring_buffer(void) = default;
		gpu_ring_buffer(const gpu_ring_buffer &) = delete;
		gpu_ring_buffer(gpu_ring_buffer &&) = default;

		bool create(ID3D12Device * device, const D3D12_HEAP_DESC * desc);

		bool allocate(ID3D12Device * device,
		              gpu_command_queue & commandQueue,
		              ID3D12Resource ** resource,
		              const D3D12_RESOURCE_DESC * desc,
		              D3D12_RESOURCE_STATES initialState,
		              const D3D12_CLEAR_VALUE * optimizedClearValue);


	private:

		struct allocated_chunk
		{
			UINT   offset;
			UINT   size;
			UINT64 frameIndex;
		};

		com_ptr<ID3D12Heap>       m_heap;

		UINT m_size;
		UINT m_alignment;
		UINT m_offset;
		UINT m_freeSpace;

		std::queue<allocated_chunk> m_allocatedChunks;


	};

}