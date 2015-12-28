#pragma once

#include <fuse/pipeline_state.hpp>
#include <fuse/geometry.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/render_resource.hpp>

namespace fuse
{

	struct debug_renderer_configuration
	{
		DXGI_FORMAT rtvFormat;
		DXGI_FORMAT dsvFormat;
	};

	class debug_renderer
	{

	public:

		debug_renderer(void) = default;
		debug_renderer(const debug_renderer &) = delete;
		debug_renderer(debug_renderer &&) = default;

		bool init(ID3D12Device * device, const debug_renderer_configuration & cfg);
		void shutdown(void);

		void render_aabb(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const aabb & boudingBox,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

		void render_lines(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const XMVECTOR * vertices,
			size_t nLines,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

	private:

		pipeline_state_template      m_debugPST;
		com_ptr<ID3D12RootSignature> m_debugRS;

		debug_renderer_configuration m_configuration;

		bool create_psos(ID3D12Device * device);

	};

}