#pragma once

#include <fuse/resource.hpp>
#include <fuse/directx_helper.hpp>

#include <cstdint>
#include <vector>

enum image_format
{
	FUSE_IMAGE_FORMAT_R8G8B8A8_UINT,
	FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT,
	FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT,
	FUSE_IMAGE_FORMAT_R8G8B8_UINT,
	FUSE_IMAGE_FORMAT_R16G16B16_FLOAT,
	FUSE_IMAGE_FORMAT_R32G32B32_FLOAT,
	FUSE_IMAGE_FORMAT_A8_UINT
};

namespace fuse
{

	DXGI_FORMAT get_dxgi_format(image_format format);
	DXGI_FORMAT get_dxgi_typeless_format(image_format format);

	class image :
		public resource
	{

	public:

		image(void) = default;
		image(const char * name, resource_loader * loader, resource_manager * owner);

		~image(void);

		bool create(image_format format, uint32_t width, uint32_t height, const void * data = nullptr);

		inline uint8_t * get_data(void) { return &m_data[0]; }
		inline const uint8_t * get_data(void) const { return &m_data[0]; }

		inline image_format get_format(void) const { return m_format; }

		inline uint32_t get_width(void) const { return m_width; }
		inline uint32_t get_height(void) const { return m_height; }

		void clear(void);

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:


		std::vector<uint8_t> m_data;
				     
		image_format         m_format;

		uint32_t             m_width;
		uint32_t             m_height;

		struct devil_initializer
		{
			devil_initializer(void);
			~devil_initializer(void);
		};

		static devil_initializer m_initializer;

	};

	typedef std::shared_ptr<image> image_ptr;

}