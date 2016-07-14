#pragma once

#include <fuse/core.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include <memory>
#include <vector>

namespace fuse
{

	class gpu_render_context :
		public singleton<gpu_render_context>
	{

	public:

		gpu_render_context(void) = default;
		gpu_render_context(const gpu_render_context &) = delete;
		gpu_render_context(gpu_render_context &&) = default;

		bool init(ID3D12Device * device, uint32_t numBuffers, uint32_t ringBufferSize, D3D12_COMMAND_QUEUE_DESC * commandQueueDesc = nullptr);
		void shutdown(void);

		inline ID3D12Device * get_device(void) { return m_device.get(); }
		inline gpu_command_queue & get_command_queue(void) { return m_commandQueue; }
		inline gpu_ring_buffer & get_ring_buffer(void) { return m_ringBuffer; }

		inline uint32_t get_buffer_index(void) { return m_bufferIndex; }

		inline gpu_graphics_command_list & get_command_list(uint32_t index) { return m_commandLists[index]; }
		inline gpu_graphics_command_list & get_command_list(void) { return m_commandLists[m_bufferIndex]; }

		inline uint32_t get_buffers_count(void) const { return m_commandLists.size(); }

		inline void advance_frame_index(void)
		{
			m_commandQueue.advance_frame_index();
			m_bufferIndex = (m_bufferIndex + 1) % get_buffers_count();
			m_commandQueue.set_aux_command_list(m_auxCommandLists[m_bufferIndex]);
		}

		void reset_command_allocators(void);

	private:

		com_ptr<ID3D12Device>                  m_device;
		gpu_command_queue                      m_commandQueue;
		gpu_ring_buffer                        m_ringBuffer;

		std::vector<gpu_graphics_command_list> m_commandLists;
		std::vector<gpu_graphics_command_list> m_auxCommandLists;

		uint32_t                               m_bufferIndex;

	};

}