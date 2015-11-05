#include <fuse/mesh.hpp>
#include <fuse/math.hpp>

using namespace fuse;

mesh::mesh(const char * name, resource_loader * loader, resource_manager * owner) :
	resource(name, loader, owner) { }

mesh::~mesh(void)
{
	clear();
}

bool mesh::create(size_t vertices, size_t triangles, unsigned int storage_semantics)
{

	clear();

	m_numVertices  = vertices;
	m_numTriangles = triangles;

	m_storageFlags = storage_semantics;

	m_indices.resize(m_numTriangles);
	m_vertices.resize(m_numVertices);

	if (has_storage_semantic(FUSE_MESH_STORAGE_NORMALS))
	{
		m_normals.resize(m_numVertices);
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
	{
		m_tangents.resize(m_numVertices);
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_BITANGENTS))
	{
		m_bitangents.resize(m_numVertices);
	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (has_storage_semantic(static_cast<mesh_storage_semantic>(texcoordSemantic)))
		{
			m_texcoords[i].resize(m_numVertices);
		}

	}

	return true;

}

uint32_t mesh::get_parameters_storage_semantic_flags(void)
{

	auto & params = get_parameters();

	boost::optional<bool> normalsOpt    = params.get_optional<bool>("normals");
	boost::optional<bool> tangentsOpt   = params.get_optional<bool>("tangents");
	boost::optional<bool> bitangentsOpt = params.get_optional<bool>("bitangents");
	boost::optional<bool> texcoords0Opt = params.get_optional<bool>("texcoords0");
	boost::optional<bool> texcoords1Opt = params.get_optional<bool>("texcoords1");

	uint32_t flags = 0;

	/* Default */

	if (!normalsOpt || *normalsOpt) flags |= FUSE_MESH_STORAGE_NORMALS;
	if (!tangentsOpt || *tangentsOpt) flags |= FUSE_MESH_STORAGE_TANGENTS;
	if (!bitangentsOpt || *bitangentsOpt) flags |= FUSE_MESH_STORAGE_TANGENTS;
	if (!texcoords0Opt || *texcoords0Opt) flags |= FUSE_MESH_STORAGE_TEXCOORDS0;

	/* Non default */

	if (texcoords1Opt && *texcoords1Opt) flags |= FUSE_MESH_STORAGE_TEXCOORDS1;

	return flags;

}

bool mesh::calculate_tangent_space(void)
{

	if (!has_storage_semantic(FUSE_MESH_STORAGE_NORMALS))
	{
		return false;
	}

	add_storage_semantic(FUSE_MESH_STORAGE_TANGENTS);
	add_storage_semantic(FUSE_MESH_STORAGE_BITANGENTS);

	//if (!has_storage_semantic(FUSE_MESH_STORAGE_TEXCOORDS0))
	{

		for (int i = 0; i < m_numVertices; i++)
		{

			float absNx = fabs(m_normals[i].x);

			XMFLOAT3 v = { absNx > .99f ? 0.f : 1.f, absNx > .99f ? 1.f : 0.f, 0.f };

			XMVECTOR N = to_vector(m_normals[i]);
			XMVECTOR T = to_vector(v);
			XMVECTOR B = XMVector3Cross(N, T);

			T = XMVector3Cross(N, B);

			m_tangents[i]   = to_float3(T);
			m_bitangents[i] = to_float3(B);

		}

	}
	//else
	{

		//throw;
		// Tangent space with texcoord

	}

	return true;

}

void mesh::clear(void)
{

	m_vertices.clear();
	m_vertices.shrink_to_fit();

	m_normals.clear();
	m_normals.shrink_to_fit();

	m_tangents.clear();
	m_tangents.shrink_to_fit();

	m_bitangents.clear();
	m_bitangents.shrink_to_fit();

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{
		m_texcoords[i].clear();
		m_texcoords[i].shrink_to_fit();
	}

	m_numVertices  = m_numTriangles = 0;
	m_storageFlags = 0;

}

bool mesh::load_impl(void)
{
	// TODO: Implement own mesh file format loader
	return false;
}

void mesh::unload_impl(void)
{

}

size_t mesh::calculate_size_impl(void)
{

	size_t indicesSize   = m_numTriangles * 3 * sizeof(uint32_t);
	size_t verticesSize  = m_numVertices * 3 * sizeof(float);
	size_t texcoordsSize = m_numVertices * 2 * sizeof(float);

	int texcoordsMultiplier = 1;
	int verticesMultiplier  = 1;

	if (has_storage_semantic(FUSE_MESH_STORAGE_NORMALS))
	{
		verticesMultiplier++;
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
	{
		verticesMultiplier++;
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_BITANGENTS))
	{
		verticesMultiplier++;
	}

	uint32_t texcoordFlag = FUSE_MESH_STORAGE_TEXCOORDS0;

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		if (m_storageFlags & texcoordFlag)
		{
			texcoordsMultiplier++;
		}

		texcoordFlag <<= 1;

	}

	return verticesSize * verticesMultiplier + texcoordsSize * texcoordsMultiplier + indicesSize;

}

bool mesh::add_storage_semantic(mesh_storage_semantic semantic)
{

	if (has_storage_semantic(semantic))
	{
		return false;
	}

	m_storageFlags |= semantic;

	recalculate_size();

	switch (semantic)
	{

	case FUSE_MESH_STORAGE_NORMALS:
		{
			m_normals.resize(m_numVertices);
			return true;
		}

	case FUSE_MESH_STORAGE_TANGENTS:
		{
			m_tangents.resize(m_numVertices);
			return true;
		}

	case FUSE_MESH_STORAGE_BITANGENTS:
		{
			m_bitangents.resize(m_numVertices);
			return true;
		}

		
	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (semantic == texcoordSemantic)
		{
			m_texcoords[i].resize(m_numVertices);
			return true;
		}

	}

	return false;

}

bool mesh::remove_storage_semantic(mesh_storage_semantic semantic)
{
	
	if (!has_storage_semantic(semantic))
	{
		return false;
	}

	m_storageFlags &= ~semantic;

	recalculate_size();

	switch (semantic)
	{

	case FUSE_MESH_STORAGE_NORMALS:
	{
		m_normals.clear();
		m_normals.shrink_to_fit();
		return true;
	}

	case FUSE_MESH_STORAGE_TANGENTS:
	{
		m_tangents.clear();
		m_tangents.shrink_to_fit();
		return true;
	}

	case FUSE_MESH_STORAGE_BITANGENTS:
	{
		m_bitangents.clear();
		m_bitangents.shrink_to_fit();
		return true;
	}


	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (semantic == texcoordSemantic)
		{
			m_texcoords[i].clear();
			m_texcoords[i].shrink_to_fit();
			return true;
		}

	}

	return false;

}