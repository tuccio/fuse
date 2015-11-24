#include <fuse/gpu_upload_manager.hpp>
#include <fuse/mipmap_generator.hpp>

#include <d3dx12.h>

using namespace fuse;

bool gpu_upload_manager::init(ID3D12Device * device, UINT commandLists)
{

	if (!mipmap_generator::get_singleton_pointer())
	{
		new mipmap_generator(device);
	}

	m_commandLists.resize(commandLists);

	for (int i = 0; i < commandLists; i++)
	{

		com_ptr<ID3D12CommandAllocator> commandAllocator;

		FUSE_HR_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
		//FUSE_HR_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[i].get(), nullptr, IID_PPV_ARGS(m_commandLists[i].get_address())));
		m_commandLists[i].init(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr);

		FUSE_HR_CHECK(commandAllocator->SetName(L"fuse_upload_manager_command_allocator"));
		FUSE_HR_CHECK(m_commandLists[i]->SetName(L"fuse_upload_manager_command_list"));

		FUSE_HR_CHECK(m_commandLists[i]->Close());

	}

	m_index = commandLists - 1;

	return true;

}


void gpu_upload_manager::shutdown(void)
{
	//m_commandAllocators.clear();
	m_commandLists.clear();
}

void gpu_upload_manager::set_command_list_index(UINT index)
{
	//FUSE_HR_CHECK(m_commandAllocators[index]->Reset());
	m_commandLists[index].reset_command_allocator();
	m_index = index;
}

void gpu_upload_manager::upload_buffer(
	gpu_command_queue & commandQueue,
	ID3D12Resource * destination,
	UINT64 destinationOffset,
	ID3D12Resource * source,
	UINT64 sourceOffset,
	UINT64 size)
{

	m_commandLists[m_index].reset_command_list(nullptr);

	//FUSE_HR_CHECK(m_commandLists[m_index]->Reset(m_commandAllocators[m_index].get(), nullptr));

	m_commandLists[m_index].resource_barrier_transition(destination, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandLists[m_index].resource_barrier_transition(source, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_commandLists[m_index]->CopyBufferRegion(destination, destinationOffset, source, sourceOffset, size);

	FUSE_HR_CHECK(m_commandLists[m_index]->Close());
	commandQueue.execute(m_commandLists[m_index]);

}

void gpu_upload_manager::upload_texture(
	gpu_command_queue & commandQueue,
	const D3D12_TEXTURE_COPY_LOCATION * destination,
	const D3D12_TEXTURE_COPY_LOCATION * source)
{

	m_commandLists[m_index].reset_command_list(nullptr);

	//FUSE_HR_CHECK(m_commandLists[m_index]->Reset(m_commandAllocators[m_index].get(), nullptr));

	m_commandLists[m_index].resource_barrier_transition(destination->pResource, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandLists[m_index].resource_barrier_transition(source->pResource, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_commandLists[m_index]->CopyTextureRegion(destination, 0, 0, 0, source, nullptr);

	FUSE_HR_CHECK(m_commandLists[m_index]->Close());

	commandQueue.execute(m_commandLists[m_index]);

}

bool gpu_upload_manager::generate_mipmaps(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	ID3D12Resource * resource)
{
	return mipmap_generator::get_singleton_pointer()->generate_mipmaps(device, commandQueue, m_commandLists[m_index], resource);
}