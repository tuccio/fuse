#pragma once

#include <fuse/pipeline_state.hpp>
#include <fuse/geometry.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/render_resource.hpp>
#include <fuse/color.hpp>

#include <vector>
#include <algorithm>
#include <iterator>

namespace fuse
{

	struct debug_renderer_configuration
	{
		DXGI_FORMAT rtvFormat;
		DXGI_FORMAT dsvFormat;
	};

	struct debug_vertex
	{

		debug_vertex(void) = default;
		debug_vertex(const debug_vertex &) = default;
		debug_vertex(const XMFLOAT3 & position, const XMFLOAT4 & color = XMFLOAT4(0, 0, 1, 1)) :
			position(position), color(color) {}

		XMFLOAT3 position;
		XMFLOAT4 color;

	};

	struct debug_line
	{
		debug_vertex v0;
		debug_vertex v1;
	};

	class debug_renderer
	{

	public:

		debug_renderer(void) = default;
		debug_renderer(const debug_renderer &) = delete;
		debug_renderer(debug_renderer &&) = default;

		bool init(ID3D12Device * device, const debug_renderer_configuration & cfg);
		void shutdown(void);

		template <typename Iterator>
		void add_lines(Iterator begin, Iterator end)
		{
			std::copy(begin, end, std::back_inserter(m_lines));
		}

		void add_aabb(const aabb & bb, const color_rgba & color);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

	private:

		pipeline_state_template      m_debugPST;
		com_ptr<ID3D12RootSignature> m_debugRS;

		debug_renderer_configuration m_configuration;

		bool create_psos(ID3D12Device * device);

		std::vector<debug_line> m_lines;

	};

}