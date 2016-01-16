#pragma once

#include <fuse/mesh.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/properties_macros.hpp>

namespace fuse
{

	class gpu_mesh :
		public resource
	{

	public:

		gpu_mesh(void) = default;
		gpu_mesh(const char_t * name, resource_loader * loader, resource_manager * owner);

		bool create(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, mesh * mesh);
		void clear(gpu_command_queue & commandQueue);

		inline const D3D12_VERTEX_BUFFER_VIEW * get_vertex_buffers(void) const { return m_vertexBuffers; }

		inline uint32_t get_num_indices(void) const { return 3 * m_numTriangles; }

		inline bool has_storage_semantic(mesh_storage_semantic semantic) const { return static_cast<bool>(semantic & m_storageFlags); }

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		uint32_t m_storageFlags;

		uint32_t m_numVertices;
		uint32_t m_numTriangles;

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

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(num_vertices, m_numVertices)
			(num_triangles, m_numTriangles)
			(storage_semantic_flags, m_storageFlags)
			(position_data, m_positionData)
			(non_position_data, m_nonPositionData)
			(index_data, m_indexData)
		)

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(resource, m_dataBuffer)
		)

	};

	typedef std::shared_ptr<gpu_mesh> gpu_mesh_ptr;

}