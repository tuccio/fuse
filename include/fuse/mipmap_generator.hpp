#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/singleton.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/compile_shader.hpp>

#include <unordered_map>
#include <vector>

namespace fuse
{

	class mipmap_generator :
		public singleton<mipmap_generator>
	{

	public:

		mipmap_generator(ID3D12Device * device);

		bool generate_mipmaps(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			ID3D12Resource * resource);

	private:

		com_ptr<ID3D12RootSignature> m_rs;
		com_ptr<ID3DBlob> m_vs;
		com_ptr<ID3DBlob> m_ps;

		com_ptr<ID3D12ShaderReflection> m_vsReflection;

		com_ptr<ID3D12Fence> m_fence;
		UINT64               m_mipID;
		HANDLE               m_hFenceEvent;

		std::unordered_map<DXGI_FORMAT, com_ptr<ID3D12PipelineState>> m_psos;

		std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayoutVector;

		bool init(ID3D12Device * device);
		bool create_pso(ID3D12Device * device, DXGI_FORMAT format);

		ID3D12PipelineState * get_pso(DXGI_FORMAT format);

	};

}