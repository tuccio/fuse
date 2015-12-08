#pragma once

#include <fuse/assimp_loader.hpp>
#include <fuse/allocators.hpp>
#include <fuse/camera.hpp>

#include "light.hpp"
#include "renderable.hpp"
#include "skybox.hpp"

#include <vector>
#include <utility>

namespace fuse
{

	using renderable_vector = std::vector<renderable*>;
	using renderable_iterator = renderable_vector::iterator;

	using camera_vector = std::vector<camera*>;
	using camera_iterator = camera_vector::iterator;

	using light_vector = std::vector<light*>;
	using light_iterator = light_vector::iterator;

	class scene
	{

	public:

		scene(void) = default;
		scene(const scene &) = delete;
		scene(scene &&) = default;

		void clear(void);

		bool import_static_objects(assimp_loader * loader);
		bool import_cameras(fuse::assimp_loader * loader);
		bool import_lights(fuse::assimp_loader * loader);

		std::pair<renderable_iterator, renderable_iterator> get_static_objects(void);
		std::pair<camera_iterator, camera_iterator> get_cameras(void);
		std::pair<light_iterator, light_iterator> get_lights(void);

		inline skybox * get_skybox(void) { return &m_skybox; }

	private:

		renderable_vector m_staticObjects;
		camera_vector     m_cameras;
		light_vector      m_lights;

		skybox            m_skybox;

		camera * m_activeCamera;

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(active_camera, m_activeCamera)
		)

	};

}