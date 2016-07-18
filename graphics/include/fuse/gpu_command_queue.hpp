#pragma once

#include <fuse/core.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/descriptor_heap.hpp>

#include <queue>
#include <unordered_map>

#include <boost/variant.hpp>

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

		bool wait_for_frame(UINT64 fenceValue, UINT time = INFINITE) const;

		inline void advance_frame_index(void) { FUSE_HR_CHECK((*this)->Signal(m_fence.get(), ++m_frameIndex)); }
		inline UINT64 get_frame_index(void) const { return m_frameIndex; }

		// Executes a command list, adding the necessary resource barriers depending on the global state
		void execute(gpu_graphics_command_list & commandList);

		// Releases a resource when the current frame is completed by the GPU
		void safe_release(IUnknown * resource) const;
		void safe_release(descriptor_heap * heap, descriptor_token_t token, uint32_t count = 1) const;

		void set_aux_command_list(gpu_graphics_command_list & commandList);

	private:

		com_ptr<ID3D12Fence>        m_fence;
		HANDLE                      m_hFenceEvent;
		UINT64                      m_frameIndex;
		mutable UINT64              m_fenceValue;

		gpu_graphics_command_list * m_auxCommandList;

		struct garbage_descriptor_type
		{
			descriptor_heap * heap;
			descriptor_token_t token;
			uint32_t count;
		};

		struct garbage_object_type
		{

			enum { GARBAGE_RESOURCE, GARBAGE_DESCRIPTOR } type;

			union
			{
				garbage_descriptor_type descriptor;
				com_ptr<IUnknown>       resource;
			};

			garbage_object_type(com_ptr<IUnknown> && resource) :
				resource(std::forward<com_ptr<IUnknown>>(resource)), type(GARBAGE_RESOURCE) { }

			garbage_object_type(garbage_descriptor_type && descriptor) :
				descriptor(std::forward<garbage_descriptor_type>(descriptor)), type(GARBAGE_DESCRIPTOR) { }

			~garbage_object_type(void);

		};

		typedef std::pair<uint32_t, garbage_object_type> garbage_type;

		mutable std::queue<garbage_type> m_garbage;

		void collect_garbage(void) const;
		

	};

}