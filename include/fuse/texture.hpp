#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/resource.hpp>
#include <fuse/image.hpp>
#include <fuse/gpu_upload_manager.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/properties_macros.hpp>

namespace fuse
{

	class texture :
		public resource
	{

	public:

		texture(void) { }

		texture(const char * name, resource_loader * loader, resource_manager * owner) :
			resource(name, loader, owner) { }

		bool create(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			image * image,
			UINT mipmaps = 1,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
			bool generateMipmaps = false);

		void clear(gpu_command_queue & commandQueue);

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		com_ptr<ID3D12Resource> m_buffer;

		uint32_t                m_width;
		uint32_t                m_height;
		uint32_t                m_mipmaps;

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(width,   m_width)
			(height,  m_height)
			(mipmaps, m_mipmaps)
		)

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(resource, m_buffer)
		)

	};

	typedef std::shared_ptr<texture> texture_ptr;

}