#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/gpu_global_resource_state.hpp>

using namespace fuse;

bool gpu_ring_buffer::init(ID3D12Device * device, UINT size)
{

	if (gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_buffer)) &&
		!FUSE_HR_FAILED(m_buffer->Map(0, &CD3DX12_RANGE(0, 0), (void**) &m_bufferMap)))
	{

		m_buffer->SetName(L"gpu_ring_buffer");

		m_bufferVAddr = m_buffer->GetGPUVirtualAddress();

		m_size       = size;
		m_offset     = 0;
		m_freeSpace  = m_size;

		return true;

	}

	return false;

}

void gpu_ring_buffer::shutdown(void)
{
	m_buffer.reset();
	m_allocatedChunks.swap(decltype(m_allocatedChunks)());
}

void * gpu_ring_buffer::allocate_constant_buffer(ID3D12Device * device,
                                                 gpu_command_queue & commandQueue,
                                                 UINT size,
                                                 D3D12_GPU_VIRTUAL_ADDRESS * outHandle,
                                                 UINT64 * offset)
{

	allocated_chunk * chunk = allocate(commandQueue, size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	if (chunk)
	{
		if (outHandle) *outHandle = m_bufferVAddr + chunk->alignedOffset;
		if (offset) *offset = chunk->alignedOffset;
		return static_cast<void*>(m_bufferMap + chunk->alignedOffset);
	}

	return nullptr;

}

void * gpu_ring_buffer::allocate_texture(ID3D12Device * device,
		                                 gpu_command_queue & commandQueue,
		                                 const D3D12_SUBRESOURCE_FOOTPRINT & footprint,
		                                 D3D12_PLACED_SUBRESOURCE_FOOTPRINT * placedTex)
{

	allocated_chunk * chunk = allocate(commandQueue, footprint.Height * footprint.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	if (chunk)
	{

		if (placedTex)
		{
			placedTex->Offset    = chunk->alignedOffset;
			placedTex->Footprint = footprint;
		}

		return static_cast<void*>(m_bufferMap + chunk->alignedOffset);

	}

	return nullptr;

}

gpu_ring_buffer::allocated_chunk * gpu_ring_buffer::allocate(gpu_command_queue & commandQueue, UINT size, UINT alignment)
{

	const UINT allocationSize = size;

	if (allocationSize > m_size)
	{
		FUSE_LOG_OPT(FUSE_LITERAL("gpu_ring_buffer"), FUSE_LITERAL("Not enough space in the ring buffer for the requested resource."));
		return false;
	}

	UINT alignedAllocationSize = FUSE_ALIGN_OFFSET(m_offset, alignment) - m_offset + size;

	while (m_freeSpace < alignedAllocationSize && !m_allocatedChunks.empty())
	{

		auto & chunk = m_allocatedChunks.front();
		commandQueue.wait_for_frame(chunk.frameIndex);

		if (chunk.offset == 0)
		{
			m_offset              = 0;
			m_freeSpace           = chunk.size;
			alignedAllocationSize = size; // Offset set back to 0, no more padding at the beginning
		}
		else
		{
			m_freeSpace += chunk.size;
		}

		m_allocatedChunks.pop();

	}

	if (m_freeSpace >= alignedAllocationSize)
	{

		allocated_chunk chunk;

		chunk.offset        = m_offset;
		chunk.alignedOffset = FUSE_ALIGN_OFFSET(m_offset, alignment);
		chunk.frameIndex    = commandQueue.get_frame_index();
		chunk.size          = alignedAllocationSize;
		chunk.alignment     = alignment;

		m_offset    += alignedAllocationSize;
		m_freeSpace -= alignedAllocationSize;

		m_allocatedChunks.push(chunk);

		return &m_allocatedChunks.back();

	}

	FUSE_LOG_OPT(FUSE_LITERAL("gpu_ring_buffer"), FUSE_LITERAL("Not enough space freed for the buffer"));

	return nullptr;

}