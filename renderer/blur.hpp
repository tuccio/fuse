#pragma once

#include <cstdint>
#include <fuse/directx_helper.hpp>

#include <fuse/gpu_command_queue.hpp>

namespace fuse
{

	class box_blur
	{
		
	public:

		box_blur(void) = default;

		bool init(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height);
		void shutdown(void);

		void render(
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			ID3D12DescriptorHeap * descriptorHeap,
			ID3D12Resource * const * buffers,
			const D3D12_GPU_DESCRIPTOR_HANDLE * srvs,
			const D3D12_GPU_DESCRIPTOR_HANDLE * uavs);

	private:

		com_ptr<ID3D12PipelineState> m_horizontalPSO;
		com_ptr<ID3D12PipelineState> m_verticalPSO;
		com_ptr<ID3D12RootSignature> m_rs;

		uint32_t m_gridWidth;
		uint32_t m_gridHeight;

		bool create_pso(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height);

	};

}