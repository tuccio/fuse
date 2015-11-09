#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/properties_macros.hpp>

#include <unordered_map>

namespace fuse
{

	class gpu_command_queue :
		public com_ptr<ID3D12CommandQueue>
	{

	public:

		gpu_command_queue(void);
		gpu_command_queue(const gpu_command_queue &) = delete;
		gpu_command_queue(gpu_command_queue &&) = default;

		~gpu_command_queue(void);

		inline const gpu_command_queue * operator& (void) const { return this; }
		inline gpu_command_queue * operator&(void) { return this; }

		bool init(ID3D12Device * device, const D3D12_COMMAND_QUEUE_DESC * queueDesc = nullptr);
		void shutdown(void);

		inline UINT64 get_fence_value(void) const { return m_fenceValue; }
		inline UINT64 update_fence_value(void) const { return m_fenceValue = m_fence->GetCompletedValue(); }

		bool wait_for_frame(UINT64 fenceValue, UINT time = INFINITE) const;

		inline void advance_frame_index(void) { FUSE_HR_CHECK((*this)->Signal(m_fence.get(), ++m_frameIndex)); }
		inline UINT64 get_frame_index(void) const { return m_frameIndex; }

		void execute(gpu_graphics_command_list & commandList);

	private:

		com_ptr<ID3D12Fence>        m_fence;
		HANDLE                      m_hFenceEvent;
		UINT64                      m_frameIndex;
		mutable UINT64              m_fenceValue;

		ID3D12CommandAllocator    * m_auxCommandAllocator;
		ID3D12GraphicsCommandList * m_auxCommandList;

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(aux_command_allocator, m_auxCommandAllocator)
			(aux_command_list, m_auxCommandList)
		)
		

	};

}