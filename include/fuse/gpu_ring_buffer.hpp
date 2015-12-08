#pragma once

#include <d3d12.h>

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/properties_macros.hpp>

#include <queue>

#define FUSE_ALIGN_OFFSET(Offset, Alignment) ((Offset + (Alignment - 1)) & ~(Alignment - 1))

namespace fuse
{

	class gpu_ring_buffer
	{

	public:

		gpu_ring_buffer(void) = default;
		gpu_ring_buffer(const gpu_ring_buffer &) = delete;
		gpu_ring_buffer(gpu_ring_buffer &&) = default;

		bool init(ID3D12Device * device, UINT size);
		void shutdown(void);

		void * allocate_constant_buffer(ID3D12Device * device,
		                                gpu_command_queue & commandQueue,
		                                UINT size,
		                                D3D12_GPU_VIRTUAL_ADDRESS * outAddress = nullptr,
		                                UINT64 * offset = nullptr);

		void * allocate_texture(ID3D12Device * device,
		                        gpu_command_queue & commandQueue,
		                        const D3D12_SUBRESOURCE_FOOTPRINT & footprint,
		                        D3D12_PLACED_SUBRESOURCE_FOOTPRINT * placedTex);


	private:

		struct allocated_chunk
		{
			UINT   offset;
			UINT   alignedOffset;
			UINT   size;
			UINT   alignment;
			UINT64 frameIndex;
		};

		com_ptr<ID3D12Resource>   m_buffer;
		uint8_t                 * m_bufferMap;

		D3D12_GPU_VIRTUAL_ADDRESS m_bufferVAddr;

		UINT m_size;
		UINT m_alignment;
		UINT m_offset;
		UINT m_freeSpace;

		std::queue<allocated_chunk> m_allocatedChunks;

		allocated_chunk * allocate(gpu_command_queue & commandQueue, UINT size, UINT alignment);

	public:

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(heap, m_buffer)
		)

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(heap_gpu_virtual_address, m_bufferVAddr)
		)

	};

}