#pragma once

#include <fuse/assimp_loader.hpp>
#include <fuse/allocators.hpp>
#include <fuse/camera.hpp>
#include <fuse/geometry/loose_octree.hpp>

#include "light.hpp"
#include "renderable.hpp"
#include "skydome.hpp"
#include "visual_debugger.hpp"

#include <vector>
#include <utility>

namespace fuse
{

	using renderable_vector = std::vector<renderable*>;
	using renderable_iterator = renderable_vector::iterator;

	using renderable_octree = loose_octree<renderable*, sphere, renderable_bounding_sphere_functor>;

	using camera_vector = std::vector<camera*>;
	using camera_iterator = camera_vector::iterator;

	using light_vector = std::vector<light*>;
	using light_iterator = light_vector::iterator;

	class alignas(16) scene
	{

	public:

		scene(void) = default;
		scene(const scene &) = delete;
		scene(scene &&) = default;

		void clear(void);

		bool import_static_objects(assimp_loader * loader);
		bool import_cameras(fuse::assimp_loader * loader);
		bool import_lights(fuse::assimp_loader * loader);

		std::pair<renderable_iterator, renderable_iterator> get_renderable_objects(void);
		std::pair<camera_iterator, camera_iterator>         get_cameras(void);
		std::pair<light_iterator, light_iterator>           get_lights(void);

		inline skydome * get_skydome(void) { return &m_skydome; }

		void recalculate_octree(void);

		renderable_vector frustum_culling(const frustum & f);

		inline void set_scene_bounds(const XMVECTOR & center, float halfExtents) { m_sceneBounds = aabb::from_center_half_extents(center, XMVectorSet(halfExtents, halfExtents, halfExtents, halfExtents)); }
		inline aabb get_scene_bounds(void) const { return m_sceneBounds; }

		void draw_octree(visual_debugger * debugger);

		aabb get_fitting_bounds(void) const;
		void fit_octree(float scale = 1.f);

	private:

		aabb              m_sceneBounds;

		renderable_vector m_renderableObjects;
		camera_vector     m_cameras;
		light_vector      m_lights;

		skydome           m_skydome;

		camera * m_activeCamera;

		renderable_octree m_octree;

	public:

		FUSE_PROPERTIES_BY_VALUE (
			(active_camera, m_activeCamera)
		)

	};

}