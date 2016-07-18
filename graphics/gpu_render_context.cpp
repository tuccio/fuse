#include <fuse/gpu_render_context.hpp>

#include <string>

using namespace fuse;

bool gpu_render_context::init(ID3D12Device * device, uint32_t numBuffers, uint32_t ringBufferSize, D3D12_COMMAND_QUEUE_DESC * commandQueueDesc)
{
	if (m_commandQueue.init(device, commandQueueDesc) &&
		m_ringBuffer.init(device, ringBufferSize))
	{
		m_commandLists.swap(decltype(m_commandLists)(numBuffers));
		m_auxCommandLists.swap(decltype(m_commandLists)(numBuffers));

		for (uint32_t i = 0; i < numBuffers; i++)
		{
			com_ptr<ID3D12CommandAllocator> commandAllocator, auxCommandAllocator;

			if (!FUSE_HR_FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))) &&
			    m_commandLists[i].init(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr) &&
			    !FUSE_HR_FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&auxCommandAllocator))) &&
				m_auxCommandLists[i].init(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, auxCommandAllocator.get(), nullptr))
			{
				std::wstring indexStr = std::to_wstring(i);

				std::wstring commandListName         = L"gpu_render_context_command_list_" + indexStr;
				std::wstring auxCommandListName      = L"gpu_render_context_aux_command_list_" + indexStr;
				std::wstring upCommandListName       = L"gpu_render_context_upload_command_list_" + indexStr;

				std::wstring commandAllocatorName    = L"gpu_render_context_command_allocator_" + indexStr;
				std::wstring auxCommandAllocatorName = L"gpu_render_context_aux_command_allocator_" + indexStr;
				std::wstring upCommandAllocatorName  = L"gpu_render_context_upload_command_allocator_" + indexStr;

				m_commandLists[i]->SetName(commandListName.c_str());
				m_auxCommandLists[i]->SetName(auxCommandListName.c_str());

				commandAllocator->SetName(commandAllocatorName.c_str());
				auxCommandAllocator->SetName(auxCommandAllocatorName.c_str());

				FUSE_HR_CHECK(m_commandLists[i]->Close());
				FUSE_HR_CHECK(m_auxCommandLists[i]->Close());
			}
			else
			{
				shutdown();
				return false;
			}
		}

		if (numBuffers)
		{
			m_commandQueue.set_aux_command_list(m_auxCommandLists[0]);
		}

		m_device = device;
	}
	else
	{
		return false;
	}

	return true;
}

void gpu_render_context::shutdown(void)
{
	m_commandLists.clear();
	m_commandQueue.shutdown();
	m_device.reset();
}

void gpu_render_context::reset_command_allocators(void)
{
	m_auxCommandLists[m_bufferIndex].reset_command_allocator();
	m_commandLists[m_bufferIndex].reset_command_allocator();
}