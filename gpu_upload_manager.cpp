#include <fuse/gpu_upload_manager.hpp>
#include <fuse/mipmap_generator.hpp>

using namespace fuse;

bool fuse::gpu_upload_buffer(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	ID3D12Resource * destination,
	UINT64 destinationOffset,
	ID3D12Resource * source,
	UINT64 sourceOffset,
	UINT64 size)
{

	commandList.resource_barrier_transition(destination, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList.resource_barrier_transition(source, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->CopyBufferRegion(destination, destinationOffset, source, sourceOffset, size);

	return true;

}

bool fuse::gpu_upload_texture(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	const D3D12_TEXTURE_COPY_LOCATION * destination,
	const D3D12_TEXTURE_COPY_LOCATION * source)
{

	commandList.resource_barrier_transition(destination->pResource, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList.resource_barrier_transition(source->pResource, D3D12_RESOURCE_STATE_GENERIC_READ);
	commandList->CopyTextureRegion(destination, 0, 0, 0, source, nullptr);

	return true;

}

bool fuse::gpu_generate_mipmaps(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	ID3D12Resource * resource)
{
	return mipmap_generator::get_singleton_pointer()->generate_mipmaps(device, commandQueue, commandList, resource);
}