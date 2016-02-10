#include "renderable.hpp"

#include <fuse/gpu_global_resource_state.hpp>

using namespace fuse;

renderable::renderable(void) :
	m_world(XMMatrixIdentity())
{

	m_loader = dynamic_resource_loader(
		[&](resource * r)
	{

		/* Creates a bounding box vertex buffer used in occlusion queries */

		auto renderContext = gpu_render_context::get_singleton_pointer();

		aabb bb = bounding_aabb(m_boundingSphere);

		auto bbMin = to_float3(bb.get_min());
		auto bbMax = to_float3(bb.get_max());

		XMFLOAT3 vertices[8] = {
			bbMin, { bbMax.x, bbMin.y, bbMin.z }, { bbMax.x, bbMax.y, bbMin.z }, { bbMin.x, bbMax.y, bbMin.z },
			bbMax, { bbMin.x, bbMax.y, bbMax.z },{ bbMin.x, bbMin.y, bbMax.z },{ bbMax.x, bbMin.y, bbMax.z }
		};

		XMUINT3 indices[12] = {
			{ 0, 1, 2 }, { 0, 2, 3 },
			{ 4, 5, 6 }, { 4, 6, 7 },
			{ 1, 2, 7 }, { 2, 4, 7 },
			{ 0, 3, 6 }, { 3, 5, 6 },
			{ 2, 4, 3 }, { 3, 4, 5 },
			{ 0, 1, 7 }, { 0, 6, 7 }
		};

		mesh occlusionBBCPU;
		occlusionBBCPU.create(_countof(vertices), _countof(indices), FUSE_MESH_STORAGE_NONE);

		memcpy(occlusionBBCPU.get_vertices(), vertices, sizeof(vertices));
		memcpy(occlusionBBCPU.get_indices(), indices, sizeof(indices));

		gpu_mesh * occlusionBB = static_cast<gpu_mesh*>(r);

		bool success = occlusionBB->create(renderContext->get_device(), renderContext->get_command_queue(), renderContext->get_command_list(), renderContext->get_ring_buffer(), &occlusionBBCPU);

		occlusionBBCPU.clear();
		return success;

	},
		[this](resource * r)
	{
		gpu_mesh * mesh = static_cast<gpu_mesh*>(r);
		mesh->clear(gpu_render_context::get_singleton_pointer()->get_command_queue());
	}
	);

	m_occlusionQueryBB = resource_factory::get_singleton_pointer()->create<gpu_mesh>(FUSE_RESOURCE_TYPE_GPU_MESH, FUSE_LITERAL(""), default_parameters(), &m_loader);

}

bool renderable::load_occlusion_resources(ID3D12Device * device)
{
	
	return
		
		m_occlusionQueryBB &&
		m_occlusionQueryBB->load() &&
		// Create the query result buffer if it does not exist
		(m_queryResult ||
		gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(8),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_queryResult)));

}

sphere renderable::get_world_space_bounding_sphere(void) const
{
	return transform_affine(m_boundingSphere, m_world);
}