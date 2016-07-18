#pragma once

#include <fuse/mesh.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

namespace fuse
{

	class mesh_manager :
		public resource_manager
	{

	public:

		mesh_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_MESH) { }

	protected:

		resource * create_impl(const char_t * name, resource_loader * loader) override { return new mesh(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}