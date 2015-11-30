#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/render_resource.hpp>

namespace fuse
{

	class tonemapper
	{

	public:

		tonemapper(void) = default;
		tonemapper(const tonemapper &) = delete;
		tonemapper(tonemapper &&) = default;

		bool init(ID3D12Device * device);

		void render(ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			const render_resource & source,
			const render_resource & destination,
			UINT width,
			UINT height);

	private:

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		bool create_pso(ID3D12Device * device);

	};

}
