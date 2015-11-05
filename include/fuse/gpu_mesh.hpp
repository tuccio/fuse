#pragma once

#include <fuse/mesh.hpp>
#include <fuse/gpu_upload_manager.hpp>

namespace fuse
{

	class gpu_mesh :
		public resource
	{

	public:

		gpu_mesh(void) = default;
		gpu_mesh(const char * name, resource_loader * loader, resource_manager * owner);
		~gpu_mesh(void);

		bool create(ID3D12Device * device, gpu_upload_manager * uploadManager, mesh * mesh);
		void clear(void);

		inline D3D12_VERTEX_BUFFER_VIEW get_position_data(void) const { return m_positionData; }
		inline D3D12_VERTEX_BUFFER_VIEW get_non_position_data(void) const { return m_nonPositionData; }

		inline D3D12_INDEX_BUFFER_VIEW get_index_data(void) const { return m_indexData; }

		inline const D3D12_VERTEX_BUFFER_VIEW * get_vertex_buffers(void) const { return m_vertexBuffers; }

		inline uint32_t get_num_vertices(void) const { return m_numVertices; }
		inline uint32_t get_num_triangles(void) const { return m_numTriangles; }
		inline uint32_t get_num_indices(void) const { return 3 * m_numTriangles; }

		inline bool     has_storage_semantic(mesh_storage_semantic semantic) const { return static_cast<bool>(semantic & m_storageFlags); }
		inline uint32_t get_storage_semantic_flags(void) const { return m_storageFlags; }

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		uint32_t                m_storageFlags;

		uint32_t                m_numVertices;
		uint32_t                m_numTriangles;

		com_ptr<ID3D12Resource> m_dataBuffer;

		union
		{

			struct
			{
				D3D12_VERTEX_BUFFER_VIEW m_positionData;
				D3D12_VERTEX_BUFFER_VIEW m_nonPositionData;
			};

			D3D12_VERTEX_BUFFER_VIEW m_vertexBuffers[2];

		};

		D3D12_INDEX_BUFFER_VIEW m_indexData;

	};

	typedef std::shared_ptr<gpu_mesh> gpu_mesh_ptr;

}