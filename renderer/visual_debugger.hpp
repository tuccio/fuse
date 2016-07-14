#pragma once

#include <fuse/color.hpp>
#include <fuse/geometry.hpp>
#include <fuse/core/properties_macros.hpp>

#include <fuse/gpu_command_queue.hpp>

#include <boost/variant.hpp>

#include <unordered_map>
#include <string>

#include "debug_renderer.hpp"

namespace fuse
{

	class visual_debugger :
		public debug_renderer
	{

	public:

		visual_debugger(void);

		bool init(ID3D12Device * device, const debug_renderer_configuration & cfg);

		void add_persistent(const char_t * name, const aabb & boundingBox, const color_rgba & color);
		void add_persistent(const char_t * name, const frustum & frustum, const color_rgba & color);
		void add_persistent(const char_t * name, const ray & ray, const color_rgba & color);

		void remove_persistent(const char_t * name);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer);

	private:

		typedef boost::variant<aabb, frustum, ray> geometry;

		struct draw_info
		{
			geometry   geometry;
			color_rgba color;
		};

		bool m_drawBoundingVolumes;
		bool m_drawOctree;
		bool m_drawSkydome;
		bool m_drawSelectedNodeBoundingVolume;

		float m_texturesScale;

		debug_renderer * m_renderer;

		std::unordered_map<string_t, draw_info> m_persistentObjects;

		void draw_persistent_objects(void);

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(draw_bounding_volumes, m_drawBoundingVolumes)
			(draw_octree, m_drawOctree)
			(draw_skydome, m_drawSkydome)
			(draw_selected_node_bounding_volume, m_drawSelectedNodeBoundingVolume)
			(textures_scale, m_texturesScale)
		)

	};

}