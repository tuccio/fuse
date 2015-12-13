#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/render_resource.hpp>

namespace fuse
{

	class sdsm
	{

	public:

		sdsm(void);
		
		bool init(ID3D12Device * device, uint32_t width, uint32_t height);
		void shutdown(void);

		void create_log_partitions(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, const render_resource & depthBuffer, const render_resource & output);

		static size_t get_constant_buffer_size(void);

	private:

		uint32_t m_width, m_height;

		com_ptr<ID3D12PipelineState> m_minMaxReductionPSO;
		com_ptr<ID3D12RootSignature> m_minMaxReductionRS;

		render_resource m_pingPongBuffer;

		bool create_psos(ID3D12Device * device);
		bool create_resources(ID3D12Device * device);


	};

}