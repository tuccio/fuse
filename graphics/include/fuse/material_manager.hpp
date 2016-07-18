#pragma once

#include <fuse/material.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

namespace fuse
{

	class material_manager :
		public resource_manager
	{

	public:

		material_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_MATERIAL) { }

	protected:

		resource * create_impl(const char_t * name, resource_loader * loader) override { return new material(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}