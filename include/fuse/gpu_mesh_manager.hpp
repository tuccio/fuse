#pragma once

#include <fuse/gpu_mesh.hpp>

#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

#include <tuple>

namespace fuse
{

	class gpu_mesh_manager :
		public resource_manager
	{

	public:

		gpu_mesh_manager(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_upload_manager * uploadManager, gpu_ring_buffer * ringBuffer) :
			m_argsTuple(device, &commandQueue, uploadManager, ringBuffer),
			resource_manager(FUSE_RESOURCE_TYPE_GPU_MESH, reinterpret_cast<args_tuple_type*>(&m_argsTuple)) { }

	protected:

		resource * create_impl(const char * name, resource_loader * loader) override { return new gpu_mesh(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	private:

		using args_tuple_type = std::tuple<ID3D12Device*, gpu_command_queue*, gpu_upload_manager*, gpu_ring_buffer*>;

		args_tuple_type m_argsTuple;

	};

}