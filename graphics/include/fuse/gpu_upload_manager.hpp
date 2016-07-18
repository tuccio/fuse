#pragma once

#include <fuse/gpu_command_queue.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/mipmap_generator.hpp>

namespace fuse
{

	bool gpu_upload_buffer(
		gpu_command_queue & commandQueue,
		gpu_graphics_command_list & commandList,
		ID3D12Resource * destination,
		UINT64 destinationOffset,
		ID3D12Resource * source,
		UINT64 sourceOffset,
		UINT64 size);

	bool gpu_upload_texture(
		gpu_command_queue & commandQueue,
		gpu_graphics_command_list & commandList,
		const D3D12_TEXTURE_COPY_LOCATION * destination,
		const D3D12_TEXTURE_COPY_LOCATION * source);

	bool gpu_generate_mipmaps(
		ID3D12Device * device,
		gpu_command_queue & commandQueue,
		gpu_graphics_command_list & commandList,
		ID3D12Resource * resource);

}