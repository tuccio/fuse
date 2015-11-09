#pragma once

#include <fuse/image.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

namespace fuse
{

	class image_manager :
		public resource_manager
	{

	public:

		image_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_IMAGE) { }

	protected:

		resource * create_impl(const char * name, resource_loader * loader) override { return new image(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}