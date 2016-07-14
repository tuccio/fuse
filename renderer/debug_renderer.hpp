#pragma once

#include <fuse/pipeline_state.hpp>
#include <fuse/geometry.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/render_resource.hpp>
#include <fuse/render_resource_manager.hpp>
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
		debug_vertex(const float3 & position, const float4 & color = float4(0, 0, 1, 1)) :
			position(position), color(color) {}

		float3 position;
		float4 color;

	};

	struct debug_line
	{
		debug_vertex v0;
		debug_vertex v1;
	};

	struct debug_texture
	{
		uint2 position;
		float scaledWidth;
		float scaledHeight;
		bool  hdr;
		render_resource_ptr texture;
	};

	class debug_renderer
	{

	public:

		debug_renderer(void) = default;
		debug_renderer(const debug_renderer &) = delete;
		debug_renderer(debug_renderer &&) = default;

		bool init(ID3D12Device * device, const debug_renderer_configuration & cfg);
		void shutdown(void);

		void add(const aabb & bb, const color_rgba & color);
		void add(const frustum & f, const color_rgba & color);
		void add(const ray & r, const color_rgba & color);

		void add(ID3D12Device * device, gpu_graphics_command_list & commandList, UINT bufferIndex, const render_resource & r, uint2 position, float scale, bool hdr);

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

		pipeline_state_template      m_debugTexturePST;
		com_ptr<ID3D12RootSignature> m_debugTextureRS;

		debug_renderer_configuration m_configuration;

		std::vector<debug_line>    m_lines;
		std::vector<debug_texture> m_textures;

		bool create_debug_pso(ID3D12Device * device);
		bool create_debug_texture_pso(ID3D12Device * device);

		void render_lines(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

		void render_textures(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

		template <typename Iterator>
		void add_lines(Iterator begin, Iterator end)
		{
			std::copy(begin, end, std::back_inserter(m_lines));
		}

	};

}