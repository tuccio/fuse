#pragma once

#include <d3d12.h>

#include <fuse/directx_helper.hpp>
#include <map>

namespace fuse
{

	class gpu_upload_manager
	{

	public:

		gpu_upload_manager(ID3D12Device * device, ID3D12CommandQueue * commandQueue);
		gpu_upload_manager(const gpu_upload_manager &) = delete;

		~gpu_upload_manager(void);

		UINT64 upload(ID3D12Device * device,
			          ID3D12Resource * dest,
			          UINT firstSubresource,
			          UINT numSubresources,
			          D3D12_SUBRESOURCE_DATA * data,
		              D3D12_RESOURCE_BARRIER * before = nullptr,
		              D3D12_RESOURCE_BARRIER * after  = nullptr);

		void collect_garbage(void);

		bool wait(UINT waitTime = INFINITE);
		bool wait_for_id(UINT64 id, UINT waitTime = INFINITE);

	private:

		com_ptr<ID3D12CommandQueue>               m_commandQueue;

		com_ptr<ID3D12Fence>                      m_fence;
		UINT64                                    m_lastAllocated;

		com_ptr<ID3D12CommandAllocator>           m_commandAllocator;
		com_ptr<ID3D12GraphicsCommandList>        m_commandList;
												  
		std::map<UINT64, com_ptr<ID3D12Resource>> m_heaps;

	};

}