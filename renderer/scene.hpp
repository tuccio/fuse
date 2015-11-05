#pragma once

#include <fuse/assimp_loader.hpp>
#include <fuse/allocators.hpp>
#include <fuse/camera.hpp>

#include "renderable.hpp"

#include <vector>
#include <utility>

class scene
{

public:

	using renderable_vector   = std::vector<renderable, fuse::aligned_allocator<renderable>>;
	using renderable_iterator = renderable_vector::iterator;
	using camera_vector       = std::vector<fuse::camera, fuse::aligned_allocator<fuse::camera>>;
	using camera_iterator     = camera_vector::iterator;

	scene(void) = default;
	scene(const scene &) = delete;
	scene(scene &&) = default;

	bool import_static_objects(fuse::assimp_loader * loader);
	bool import_cameras(fuse::assimp_loader * loader);

	std::pair<renderable_iterator, renderable_iterator> get_static_objects_iterators(void);
	std::pair<camera_iterator, camera_iterator> get_cameras_iterators(void);

private:

	renderable_vector   m_staticObjects;
	camera_vector       m_cameras;

	fuse::camera      * m_activeCamera;

public:

	FUSE_PROPERTIES_BY_VALUE(
		(active_camera, m_activeCamera)
	)

};