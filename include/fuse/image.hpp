#pragma once

#include <fuse/resource.hpp>

#include <IL/il.h>

#include <cstdint>

enum image_format
{
	FUSE_IMAGE_FORMAT_R8G8B8A8_UINT,
	FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT,
	FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT,
	FUSE_IMAGE_FORMAT_R8G8B8_UINT,
	FUSE_IMAGE_FORMAT_R16G16B16_FLOAT,
	FUSE_IMAGE_FORMAT_R32G32B32_FLOAT
};

namespace fuse
{

	class image :
		public resource
	{

	public:

		image(void);
		image(const char * name, resource_loader * loader, resource_manager * owner);

		~image(void);

		inline uint8_t * get_data(void) const { return m_data; }

		inline image_format get_format(void) const { return m_format; }

		inline uint32_t get_width(void) const { return m_width; }
		inline uint32_t get_height(void) const { return m_height; }

		void clear(void);

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		ILuint         m_handle;
		image_format   m_format;

		uint8_t      * m_data;
				     
		uint32_t       m_width;
		uint32_t       m_height;

	};

}