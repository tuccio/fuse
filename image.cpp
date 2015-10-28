#include <fuse/image.hpp>
#include <fuse/logger.hpp>

#include <IL/ilu.h>

using namespace fuse;

image::image(void) :
	m_handle(IL_INVALID_VALUE) { }

image::image(const char * name, resource_loader * loader, resource_manager * owner) :
	resource(name, loader, owner) { }

image::~image(void)
{
	clear();
}

bool image::load_impl(void)
{

	bool loaded;

	clear();

	m_handle = ilGenImage();

	if (m_handle != IL_INVALID_VALUE)
	{

		ilBindImage(m_handle);

		bool converted = true;

		if (ilLoadImage(get_name()))
		{

			ILenum formatChannels;
			ILenum formatDataType;

			auto format = get_parameters().get_optional<int>("format");

			if (format)
			{

				switch (*format)
				{

				case FUSE_IMAGE_FORMAT_R16G16B16_FLOAT:
					formatChannels = IL_RGB;
					formatDataType = IL_HALF;
					m_format       = FUSE_IMAGE_FORMAT_R16G16B16_FLOAT;
					break;

				case FUSE_IMAGE_FORMAT_R32G32B32_FLOAT:
					formatChannels = IL_RGB;
					formatDataType = IL_FLOAT;
					m_format       = FUSE_IMAGE_FORMAT_R32G32B32_FLOAT;
					break;

				case FUSE_IMAGE_FORMAT_R8G8B8_UINT:
					formatChannels = IL_RGB;
					formatDataType = IL_BYTE;
					m_format       = FUSE_IMAGE_FORMAT_R8G8B8_UINT;
					break;

				case FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT:
					formatChannels = IL_RGBA;
					formatDataType = IL_HALF;
					m_format       = FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT;
					break;

				case FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT:
					formatChannels = IL_RGBA;
					formatDataType = IL_FLOAT;
					m_format       = FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT;
					break;

				case FUSE_IMAGE_FORMAT_R8G8B8A8_UINT:
				default:
					formatChannels = IL_RGBA;
					formatDataType = IL_BYTE;
					m_format       = FUSE_IMAGE_FORMAT_R8G8B8A8_UINT;
					break;

				}

				converted = ilConvertImage(formatChannels, formatDataType);

			}

			if (converted)
			{

				if (ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT)
				{
					iluFlipImage();
				}

				loaded = true;

				m_width  = ilGetInteger(IL_IMAGE_WIDTH);
				m_height = ilGetInteger(IL_IMAGE_HEIGHT);

			}

		}
		else
		{
			FUSE_LOG_OPT_DEBUG(std::stringstream() << "Unable to load image file \"" << get_name() << "\".");
		}

	}

	if (!loaded) clear();

	return loaded;

}

void image::unload_impl(void)
{
	clear();
}

size_t image::calculate_size_impl(void)
{
	ilBindImage(m_handle);
	return ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
}

void image::clear(void)
{

	if (m_handle != IL_INVALID_VALUE)
	{
		ilDeleteImage(m_handle);
		m_handle = IL_INVALID_VALUE;
	}

}