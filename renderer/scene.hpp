#pragma once

#include <fuse/core.hpp>
#include <fuse/assimp_loader.hpp>
#include <fuse/camera.hpp>
#include <fuse/geometry/loose_octree.hpp>
#include <fuse/scene_graph.hpp>

#include "light.hpp"
#include "skydome.hpp"
#include "visual_debugger.hpp"

#include <vector>
#include <utility>

namespace fuse
{

	struct geometry_bounding_sphere
	{
		inline sphere operator() (scene_graph_geometry * g) const
		{
			return g->get_global_bounding_sphere();
		}
	};

	using geometry_octree   = loose_octree<scene_graph_geometry*, sphere, geometry_bounding_sphere>;
	using geometry_vector   = std::vector<scene_graph_geometry*>;
	using geometry_iterator = geometry_vector::iterator;

	using camera_vector   = std::vector<scene_graph_camera*>;
	using camera_iterator = camera_vector::iterator;

	using light_vector   = std::vector<light*>;
	using light_iterator = light_vector::iterator;
	
	class alignas(16) scene;

	struct scene_listener
	{
		virtual void on_scene_active_camera_change(scene * scene, scene_graph_camera * oldCamera, scene_graph_camera * newCamera) {}
		virtual void on_scene_clear(scene * scene) {}
	};

	class alignas(16) scene :
		public scene_graph_node_listener
	{

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		scene(void);
		scene(const scene &) = delete;
		scene(scene &&) = default;

		void clear(void);

		bool import(assimp_loader * loader);
		bool import_cameras(fuse::assimp_loader * loader);
		bool import_lights(fuse::assimp_loader * loader);

		std::pair<geometry_iterator, geometry_iterator> get_geometry(void);
		std::pair<camera_iterator, camera_iterator>     get_cameras(void);
		std::pair<light_iterator, light_iterator>       get_lights(void);

		inline skydome * get_skydome(void) { return &m_skydome; }

		void recalculate_octree(void);

		geometry_vector frustum_culling(const frustum & f);

		inline void set_scene_bounds(const vec128 & center, float halfExtents) { m_sceneBounds = aabb::from_center_half_extents(center, vec128_set(halfExtents, halfExtents, halfExtents, halfExtents)); }
		inline aabb get_scene_bounds(void) const { return m_sceneBounds; }

		void draw_octree(visual_debugger * debugger);

		aabb get_fitting_bounds(void) const;
		void fit_octree(float scale = 1.f);

		scene_graph * get_scene_graph(void)
		{
			return &m_sceneGraph;
		}

		const scene_graph * get_scene_graph(void) const
		{
			return &m_sceneGraph;
		}

		void set_active_camera_node(scene_graph_camera * camera);
		void add_listener(scene_listener * listener);
		void remove_listener(scene_listener * listener);

		void update(void);

		void on_scene_graph_node_move(scene_graph_node * node, const mat128 & oldTransform, const mat128 & newTransform);

		bool FUSE_VECTOR_CALL ray_pick(ray r, scene_graph_geometry * & node, float & t);

	private:

		aabb            m_sceneBounds;

		geometry_vector m_geometry;
		camera_vector   m_cameras;
		light_vector    m_lights;

		skydome         m_skydome;

		geometry_octree m_octree;
		scene_graph     m_sceneGraph;

		scene_graph_camera * m_activeCamera;
		bool m_boundsGrowth;

		std::vector<scene_listener*> m_listeners;

	public:

		FUSE_PROPERTIES_BY_VALUE (
			(bounds_growth, m_boundsGrowth)
		)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY (
			(cameras, m_cameras)
			(active_camera_node, m_activeCamera)
		)

	private:

		void on_geometry_add(scene_graph_geometry * g);
		void on_geometry_remove(scene_graph_geometry * g);

		void create_scene_graph_assimp(assimp_loader * loader, const aiScene * scene, aiNode * node, scene_graph_node * parent);

	};

}