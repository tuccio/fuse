#include <fuse/gpu_mesh.hpp>
#include <fuse/resource_factory.hpp>
#include <fuse/gpu_global_resource_state.hpp>
#include <fuse/gpu_render_context.hpp>
#include <fuse/gpu_upload_manager.hpp>

#include <boost/iterator.hpp>

#include <algorithm>
#include <memory>
#include <tuple>

using namespace fuse;

gpu_mesh::gpu_mesh(const char_t * name, resource_loader * loader, resource_manager * owner) :
	resource(name, loader, owner) { }

bool gpu_mesh::create(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, mesh * mesh)
{
	m_numVertices  = mesh->get_num_vertices();
	m_numTriangles = mesh->get_num_triangles();
	m_storageFlags = mesh->get_storage_semantic_flags();

	size_t bufferSize = mesh->get_size();
	
	std::unique_ptr<uint8_t[]> intermediateBuffer(new uint8_t[bufferSize]);

	size_t offset = 0;

	// Copy all the vertices

	uint8_t * bufferPosition = intermediateBuffer.get();
	float * tmp = (float*)bufferPosition;

	size_t verticesBufferSize = mesh->get_num_vertices() * 3 * sizeof(float);
	offset += verticesBufferSize;

	std::memcpy(bufferPosition, mesh->get_vertices(), verticesBufferSize);
	bufferPosition += verticesBufferSize;

	// Then copy the indices

	size_t indicesBufferSize = mesh->get_num_triangles() * 3 * sizeof(uint32_t);

	offset += indicesBufferSize;

	std::memcpy(bufferPosition, mesh->get_indices(), indicesBufferSize);
	bufferPosition += indicesBufferSize;

	// Now we need to interleave the non vertex data

	std::vector<uint8_t*> iterators;
	std::vector<uint32_t> dataSize;

	if (has_storage_semantic(FUSE_MESH_STORAGE_NORMALS))
	{
		iterators.push_back(reinterpret_cast<uint8_t*>(mesh->get_normals()));
		dataSize.push_back(3 * sizeof(float));
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
	{
		iterators.push_back(reinterpret_cast<uint8_t*>(mesh->get_tangents()));
		dataSize.push_back(3 * sizeof(float));
	}

	if (has_storage_semantic(FUSE_MESH_STORAGE_BITANGENTS))
	{
		iterators.push_back(reinterpret_cast<uint8_t*>(mesh->get_bitangents()));
		dataSize.push_back(3 * sizeof(float));
	}

	for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
	{

		uint32_t texcoordSemantic = FUSE_MESH_STORAGE_TEXCOORDS0 << i;

		if (has_storage_semantic(static_cast<mesh_storage_semantic>(texcoordSemantic)))
		{
			iterators.push_back(reinterpret_cast<uint8_t*>(mesh->get_texcoords(i)));
			dataSize.push_back(2 * sizeof(float));
		}

	}

	for (int vertexIndex = 0; vertexIndex < m_numVertices; vertexIndex++)
	{
		for (int iteratorIndex = 0; iteratorIndex < iterators.size(); iteratorIndex++)
		{
			uint32_t copySize = dataSize[iteratorIndex];

			std::memcpy(bufferPosition, iterators[iteratorIndex], copySize);
			bufferPosition += copySize;

			offset                   += copySize;
			iterators[iteratorIndex] += copySize;
		}
	}

	// Now create the vertex buffer and upload the data

	if (gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_dataBuffer)))
	{
		UINT64 heapOffset;
		void * data = ringBuffer.allocate_constant_buffer(device, commandQueue, bufferSize, nullptr, &heapOffset);

		memcpy(data, intermediateBuffer.get(), bufferSize);

		gpu_upload_buffer(commandQueue, commandList, m_dataBuffer.get(), 0, ringBuffer.get_heap(), heapOffset, bufferSize);

		commandList.resource_barrier_transition(m_dataBuffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

		// Finally setup views

		D3D12_GPU_VIRTUAL_ADDRESS dataAddress = m_dataBuffer->GetGPUVirtualAddress();

		m_positionData.BufferLocation = dataAddress;
		m_positionData.StrideInBytes  = sizeof(float) * 3;
		m_positionData.SizeInBytes    = verticesBufferSize;

		m_indexData.BufferLocation = m_positionData.BufferLocation + m_positionData.SizeInBytes;
		m_indexData.SizeInBytes    = indicesBufferSize;
		m_indexData.Format         = DXGI_FORMAT_R32_UINT;

		uint32_t stride = 0;
		std::for_each(dataSize.begin(), dataSize.end(), [&stride](uint32_t x) { stride += x; });

		m_nonPositionData.BufferLocation = m_indexData.BufferLocation + m_indexData.SizeInBytes;
		m_nonPositionData.StrideInBytes  = stride;
		m_nonPositionData.SizeInBytes    = stride * m_numVertices;

		recalculate_size();

		return true;
	}

	return false;
}

void gpu_mesh::clear(gpu_command_queue & commandQueue)
{
	commandQueue.safe_release(m_dataBuffer.get());
	m_dataBuffer.reset();
}

bool gpu_mesh::load_impl(void)
{
	mesh_ptr m = resource_factory::get_singleton_pointer()->
		create<mesh>(FUSE_RESOURCE_TYPE_MESH, get_name());

	if (m && m->load())
	{
		auto renderContext = gpu_render_context::get_singleton_pointer();
		return create(renderContext->get_device(), renderContext->get_command_queue(), renderContext->get_command_list(), renderContext->get_ring_buffer(), m.get());
	}

	return false;
}

void gpu_mesh::unload_impl(void)
{
	auto renderContext = gpu_render_context::get_singleton_pointer();
	clear(renderContext->get_command_queue());
}

size_t gpu_mesh::calculate_size_impl(void)
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