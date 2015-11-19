#pragma once

#include <fuse/gpu_command_queue.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_graphics_command_list.hpp>

#include <vector>

namespace fuse
{

	class gpu_upload_manager
	{

	public:

		gpu_upload_manager(void) = default;
		gpu_upload_manager(const gpu_upload_manager &) = delete;

		~gpu_upload_manager(void) = default;

		bool init(ID3D12Device * device, UINT commandLists);
		void shutdown(void);

		void set_command_list_index(UINT index);

		void upload_buffer(
			gpu_command_queue & commandQueue,
			ID3D12Resource * destination,
			UINT64 destinationOffset,
			ID3D12Resource * source,
			UINT64 sourceOffset,
			UINT64 size);

		void upload_texture(
			gpu_command_queue & commandQueue,
			const D3D12_TEXTURE_COPY_LOCATION * destination,
			const D3D12_TEXTURE_COPY_LOCATION * source);

		bool generate_mipmaps(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			ID3D12Resource * resource);

	private:

		std::vector<com_ptr<ID3D12CommandAllocator>>   m_commandAllocators;
		std::vector<gpu_graphics_command_list>         m_commandLists;

		UINT                                           m_index;

	};

}