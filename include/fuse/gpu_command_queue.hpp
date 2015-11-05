#pragma once

#include <fuse/directx_helper.hpp>

#include <d3d12.h>

namespace fuse
{

	class gpu_command_queue :
		public com_ptr<ID3D12CommandQueue>
	{

	public:

		gpu_command_queue(void);
		gpu_command_queue(const gpu_command_queue &) = delete; // Might add the copy, but the copy should create a new event
		gpu_command_queue(gpu_command_queue &&) = default;

		~gpu_command_queue(void);

		bool init(ID3D12Device * device, const D3D12_COMMAND_QUEUE_DESC * queueDesc = nullptr);
		void shutdown(void);

		inline UINT64 get_fence_value(void) const { return m_fenceValue; }
		inline UINT64 update_fence_value(void) const { return m_fenceValue = m_fence->GetCompletedValue(); }

		bool wait_for_frame(UINT64 fenceValue, UINT time = INFINITE) const;

		inline void advance_frame_index(void) { FUSE_HR_CHECK((*this)->Signal(m_fence.get(), ++m_frameIndex)); }
		inline UINT64 get_frame_index(void) const { return m_frameIndex; }

	private:

		com_ptr<ID3D12Fence> m_fence;
		HANDLE               m_hFenceEvent;
		UINT64               m_frameIndex;
		mutable UINT64       m_fenceValue;

	};

}