#pragma once

#include <fuse/resource.hpp>

#include <cstdint>
#include <memory>

#define FUSE_MESH_MAX_TEXCOORDS 8

enum mesh_storage_semantic
{
	FUSE_MESH_STORAGE_NONE = 0,
	FUSE_MESH_STORAGE_NORMALS = 1,
	FUSE_MESH_STORAGE_TANGENTS = 2,
	FUSE_MESH_STORAGE_BITANGENTS = 4,
	FUSE_MESH_STORAGE_TANGENTSPACE = 7,
	FUSE_MESH_STORAGE_TEXCOORDS0 = 8,
	FUSE_MESH_STORAGE_TEXCOORDS1 = 16,
	FUSE_MESH_STORAGE_TEXCOORDS2 = 32,
	FUSE_MESH_STORAGE_TEXCOORDS3 = 64,
	FUSE_MESH_STORAGE_TEXCOORDS4 = 128,
	FUSE_MESH_STORAGE_TEXCOORDS5 = 256,
	FUSE_MESH_STORAGE_TEXCOORDS6 = 1024,
	FUSE_MESH_STORAGE_TEXCOORDS7 = 2048,
};

namespace fuse
{

	class mesh :
		public resource
	{

	public:

		mesh(void) = default;
		mesh(const char * name, resource_loader * loader, resource_manager * owner);
		~mesh(void);

		bool create(size_t vertices, size_t triangles, unsigned int storage_semantics);
		void clear(void);

		bool add_storage_semantic(mesh_storage_semantic semantic);
		bool remove_storage_semantic(mesh_storage_semantic semantic);

		inline uint32_t get_num_triangles(void) const { return m_numTriangles; }
		inline uint32_t get_num_vertices(void) const { return m_numVertices; }
		inline uint32_t get_num_indices(void) const { return 3 * m_numTriangles; }

		inline bool     has_storage_semantic(mesh_storage_semantic semantic) const { return static_cast<bool>(semantic & m_storageFlags);  }
		inline uint32_t get_storage_semantic_flags(void) const { return m_storageFlags; }

		inline float * get_vertices(void) const { return m_vertices.get(); }
		inline float * get_normals(void) const { return m_normals.get(); }
		inline float * get_tangents(void) const { return m_tangents.get(); }
		inline float * get_bitangents(void) const { return m_bitangents.get(); }

		inline float * get_texcoords(int i) const { return m_texcoords[i].get(); }

		inline uint32_t * get_indices(void) const { return m_indices.get(); }

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		std::unique_ptr<float[]>    m_vertices;
		std::unique_ptr<float[]>    m_normals;
		std::unique_ptr<float[]>    m_tangents;
		std::unique_ptr<float[]>    m_bitangents;
		std::unique_ptr<float[]>    m_texcoords[FUSE_MESH_MAX_TEXCOORDS];
		std::unique_ptr<uint32_t[]> m_indices;

		uint32_t   m_numVertices;
		uint32_t   m_numTriangles;

		uint32_t   m_storageFlags;

	};

}