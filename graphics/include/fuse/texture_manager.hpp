#include <fuse/texture.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>

namespace fuse
{

	class texture_manager :
		public resource_manager
	{

	public:

		texture_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_TEXTURE) { }

	protected:

		resource * create_impl(const char_t * name, resource_loader * loader) override { return new texture(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	};

}