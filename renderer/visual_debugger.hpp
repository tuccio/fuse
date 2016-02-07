#pragma once

#include <fuse/color.hpp>
#include <fuse/geometry.hpp>
#include <fuse/properties_macros.hpp>

#include <boost/variant.hpp>

#include <unordered_map>
#include <string>

#include "debug_renderer.hpp"

namespace fuse
{

	class visual_debugger
	{

	public:

		visual_debugger(void);

		bool init(debug_renderer * renderer);
		void shutdown(void);

		void add_persistent(const char * name, const aabb & boundingBox, const color_rgba & color);
		void remove_presistent(const char * name);

		void add(const aabb & boundingBox, const color_rgba & color);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

	private:

		typedef boost::variant<aabb> geometry;

		struct draw_info
		{
			geometry   geometry;
			color_rgba color;
		};

		bool m_drawBoundingVolumes;

		debug_renderer * m_renderer;

		std::unordered_map<std::string, draw_info> m_persistentObjects;

		void draw_persistent_objects(void);

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(draw_bounding_volumes, m_drawBoundingVolumes)
		)

	};

}