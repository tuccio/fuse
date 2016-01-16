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

		gpu_mesh_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_GPU_MESH) { }

	protected:

		resource * create_impl(const char_t * name, resource_loader * loader) override { return new gpu_mesh(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}