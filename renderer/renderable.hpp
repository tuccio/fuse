#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>
#include <fuse/gpu_mesh.hpp>
#include <fuse/material.hpp>

#include <fuse/properties_macros.hpp>

#include <fuse/geometry.hpp>

namespace fuse
{

	class alignas(16) renderable
	{

	public:

		renderable(void) : m_world(XMMatrixIdentity()) { }
		renderable(const renderable &) = default;
		renderable(renderable &&) = default;

	private:

		XMMATRIX     m_world;

		sphere       m_boundingSphere;

		gpu_mesh_ptr m_mesh;
		material_ptr m_material;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(mesh, m_mesh)
			(material, m_material)
			(world_matrix, m_world)
			(bounding_sphere, m_boundingSphere)
		)

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

	};

}

