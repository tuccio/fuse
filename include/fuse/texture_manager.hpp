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

		texture_manager(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_upload_manager * uploadManager, gpu_ring_buffer * ringBuffer) :
			m_argsTuple(device, &commandQueue, uploadManager, ringBuffer),
			resource_manager(FUSE_RESOURCE_TYPE_TEXTURE, reinterpret_cast<args_tuple_type*>(&m_argsTuple)) { }

		texture_manager(void) :
			resource_manager(FUSE_RESOURCE_TYPE_TEXTURE) { }

	protected:

		resource * create_impl(const char * name, resource_loader * loader) override { return new texture(name, loader, this); }
		void       free_impl(resource * resource) override { delete resource; }

	private:

		using args_tuple_type = std::tuple<ID3D12Device*, gpu_command_queue*, gpu_upload_manager*, gpu_ring_buffer*>;

		args_tuple_type m_argsTuple;

	};

}