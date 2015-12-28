#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>
#include <fuse/gpu_mesh.hpp>
#include <fuse/material.hpp>

#include <fuse/properties_macros.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/geometry.hpp>
#include <fuse/gpu_render_context.hpp>

namespace fuse
{

	class alignas(16) renderable
	{

	public:

		renderable(void);

		renderable(const renderable &) = default;
		renderable(renderable &&) = default;

		bool load_occlusion_resources(ID3D12Device * device);

	private:

		XMMATRIX     m_world;

		sphere       m_boundingSphere;

		gpu_mesh_ptr            m_occlusionQueryBB;
		com_ptr<ID3D12Resource> m_queryResult;

		gpu_mesh_ptr m_mesh;
		material_ptr m_material;

		dynamic_resource_loader m_loader;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(mesh,                   m_mesh)
			(material,               m_material)
			(world_matrix,           m_world)
			(bounding_sphere,        m_boundingSphere)
			(occlusion_bounding_box, m_occlusionQueryBB)
			(query_result,           m_queryResult)
		)

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

	};

	struct renderable_bounding_sphere_functor
	{
		inline sphere operator() (const renderable * r) const
		{
			return transform_affine(r->get_bounding_sphere(), r->get_world_matrix());
		}
	};

}

