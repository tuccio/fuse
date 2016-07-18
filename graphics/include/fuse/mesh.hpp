#pragma once

#include <fuse/resource.hpp>
#include <fuse/math.hpp>

#include <cstdint>
#include <memory>
#include <vector>

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
		mesh(const char_t * name, resource_loader * loader, resource_manager * owner);
		~mesh(void);

		bool create(size_t vertices, size_t triangles, unsigned int storage_semantics);
		void clear(void);

		bool add_storage_semantic(mesh_storage_semantic semantic);
		bool remove_storage_semantic(mesh_storage_semantic semantic);

		bool calculate_tangent_space(void);

		inline uint32_t get_num_triangles(void) const { return m_numTriangles; }
		inline uint32_t get_num_vertices(void) const { return m_numVertices; }
		inline uint32_t get_num_indices(void) const { return 3 * m_numTriangles; }

		inline bool     has_storage_semantic(mesh_storage_semantic semantic) const { return (semantic & m_storageFlags) != 0;  }
		inline uint32_t get_storage_semantic_flags(void) const { return m_storageFlags; }

		uint32_t get_parameters_storage_semantic_flags(void);

		inline float * get_vertices(void) const { return &m_vertices[0].x; }
		inline float * get_normals(void) const { return &m_normals[0].x; }
		inline float * get_tangents(void) const { return &m_tangents[0].x; }
		inline float * get_bitangents(void) const { return &m_bitangents[0].x; }

		inline float * get_texcoords(int i) const { return &m_texcoords[i][0].x; }

		inline uint32_t * get_indices(void) const { return &m_indices[0].x; }

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		mutable std::vector<float3> m_vertices;
		mutable std::vector<float3> m_normals;
		mutable std::vector<float3> m_tangents;
		mutable std::vector<float3> m_bitangents;
		mutable std::vector<float2> m_texcoords[FUSE_MESH_MAX_TEXCOORDS];
		mutable std::vector<uint3>  m_indices;

		uint32_t   m_numVertices;
		uint32_t   m_numTriangles;

		uint32_t   m_storageFlags;

	};

	typedef std::shared_ptr<mesh> mesh_ptr;

}