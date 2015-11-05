#pragma once

#include <fuse/math.hpp>
#include <fuse/gpu_mesh.hpp>
#include <fuse/material.hpp>

#include <fuse/properties_macros.hpp>

class alignas(16) renderable
{

public:

	renderable(void) : m_world(DirectX::XMMatrixIdentity()) { }
	renderable(const renderable &) = default;
	renderable(renderable &&) = default;

private:

	DirectX::XMMATRIX  m_world;

	fuse::gpu_mesh_ptr m_mesh;
	fuse::material_ptr m_material;

public:

	FUSE_PROPERTIES_BY_CONST_REFERENCE(
		(mesh, m_mesh)
		(material, m_material)
		(world_matrix, m_world)
	)

};