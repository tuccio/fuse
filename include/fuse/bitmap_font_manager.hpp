#pragma once

#include <fuse/bitmap_font.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

namespace fuse
{

	class bitmap_font_manager :
		public resource_manager
	{

	public:

		bitmap_font_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_BITMAP_FONT) { }

	protected:

		resource * create_impl(const char_t * name, resource_loader * loader) override { return new bitmap_font(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}