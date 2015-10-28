#include <fuse/mesh.hpp>

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

	size_t vertexBufferSize  = 3 * vertices;
	size_t indicesBufferSize = 3 * triangles;
	size_t textureBufferSize = 2 * vertices;

	m_indices  = std::unique_ptr<uint32_t[]>(new uint32_t[indicesBufferSize]);
	m_vertices = std::unique_ptr<float[]>(new float[vertexBufferSize]);

	if (has_storage_semantic(FUSE_MESH_STORAGE_NORMALS))
	{
		m_normals = std::unique_ptr<float[]>(new float[vertexBufferSize]);
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
	{
		m_tangents = std::unique_ptr<float[]>(new float[vertexBufferSize]);
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_BITANGENTS))
	{
		m_bitangents = std::unique_ptr<float[]>(new float[vertexBufferSize]);
	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		static uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (has_storage_semantic(static_cast<mesh_storage_semantic>(texcoordSemantic)))
		{
			m_texcoords[i] = std::unique_ptr<float[]>(new float[textureBufferSize]);
		}

	}

	return true;

}

void mesh::clear(void)
{

	m_vertices.reset();
	m_normals.reset();
	m_tangents.reset();
	m_bitangents.reset();

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{
		m_texcoords[i].reset();
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

	switch (semantic)
	{

	case FUSE_MESH_STORAGE_NORMALS:
		{
			size_t vertexBufferSize = m_numVertices * 3;
			m_normals = std::unique_ptr<float[]>(new float[vertexBufferSize]);
			return true;
		}

	case FUSE_MESH_STORAGE_TANGENTS:
		{
			size_t vertexBufferSize = m_numVertices * 3;
			m_tangents = std::unique_ptr<float[]>(new float[vertexBufferSize]);
			return true;
		}

	case FUSE_MESH_STORAGE_BITANGENTS:
		{
			size_t vertexBufferSize = m_numVertices * 3;
			m_bitangents = std::unique_ptr<float[]>(new float[vertexBufferSize]);
			return true;
		}

		
	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		static uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (semantic == texcoordSemantic)
		{
			size_t textureBufferSize = m_numVertices * 2;
			m_texcoords[i] = std::unique_ptr<float[]>(new float[textureBufferSize]);
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

	switch (semantic)
	{

	case FUSE_MESH_STORAGE_NORMALS:
	{
		m_normals.reset();
		return true;
	}

	case FUSE_MESH_STORAGE_TANGENTS:
	{
		m_tangents.reset();
		return true;
	}

	case FUSE_MESH_STORAGE_BITANGENTS:
	{
		m_bitangents.reset();
		return true;
	}


	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		static uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (semantic == texcoordSemantic)
		{
			m_texcoords[i].reset();
			return true;
		}

	}

	return false;

}