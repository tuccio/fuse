#pragma once

#include <fuse/core.hpp>
#include <fuse/directx_helper.hpp>

#include <cassert>
#include <unordered_map>

namespace fuse
{

	class gpu_global_resource_state :
		public singleton<gpu_global_resource_state>
	{

	public:

		inline D3D12_RESOURCE_STATES get_state(ID3D12Resource * resource)
		{
			auto it = m_globalState.find(resource);
			assert(it != m_globalState.end() && "Unregistered resource accessed.");
			return it->second;
		}

		inline void set_state(ID3D12Resource * resource, D3D12_RESOURCE_STATES state)
		{
			m_globalState[resource] = state;
		}

		inline bool create_committed_resource(
			ID3D12Device * pDevice,
			_In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
			D3D12_HEAP_FLAGS HeapFlags,
			_In_  const D3D12_RESOURCE_DESC *pResourceDesc,
			D3D12_RESOURCE_STATES InitialResourceState,
			_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
			REFIID riidResource,
			_COM_Outptr_opt_  void **ppvResource)
		{
			HRESULT hr = pDevice->CreateCommittedResource(pHeapProperties, HeapFlags, pResourceDesc, InitialResourceState, pOptimizedClearValue, riidResource, ppvResource);

			if (!FUSE_HR_FAILED(hr))
			{
				set_state(*(ID3D12Resource**)ppvResource, InitialResourceState);
				return true;
			}

			return false;
		}

	private:

		std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> m_globalState;

	};

}