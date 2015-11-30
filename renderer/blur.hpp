#pragma once

#include <cstdint>
#include <fuse/directx_helper.hpp>

#include <fuse/gpu_command_queue.hpp>
#include <fuse/render_resource.hpp>

namespace fuse
{

	class blur
	{
		
	public:

		blur(void) = default;

		bool init_box_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height);
		bool init_gaussian_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, float sigma, uint32_t width, uint32_t height);
		bool init_bilateral_blur(ID3D12Device * device, const char * type, uint32_t kernelSize, float sigma, uint32_t width, uint32_t height);
		void shutdown(void);

		void render(
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			const render_resource & source,
			const render_resource & pingPongBuffer);

	private:

		com_ptr<ID3D12PipelineState> m_horizontalPSO;
		com_ptr<ID3D12PipelineState> m_verticalPSO;
		com_ptr<ID3D12RootSignature> m_rs;

		uint32_t m_gridWidth;
		uint32_t m_gridHeight;

		bool create_box_pso(ID3D12Device * device, const char * type, uint32_t kernelSize, uint32_t width, uint32_t height);
		bool create_guassian_pso(ID3D12Device * device, const char * type, uint32_t kernelSize, float sigma, uint32_t width, uint32_t height);

	};

}