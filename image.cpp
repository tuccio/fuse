#include <fuse/image.hpp>
#include <fuse/logger.hpp>

#include <IL/ilu.h>

#include <utility>

using namespace fuse;

image::devil_initializer image::m_initializer;

static std::pair<ILenum, ILenum> get_image_devil_format(image_format format);

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

		bool exception = false;

		if (ilLoadImage(get_name()))
		{

			ILenum formatChannels;
			ILenum formatDataType;

			auto format = get_parameters().get_optional<int>("format");

			if (!format) format = FUSE_IMAGE_FORMAT_R8G8B8A8_UINT;

			m_format = static_cast<image_format>(*format);
			auto ilFormat = get_image_devil_format(m_format);

			if (!ilFormat.first)
			{
				ilFormat.first = IL_RGBA;
				ilFormat.second = IL_BYTE;
				m_format = FUSE_IMAGE_FORMAT_R8G8B8A8_UINT;
			}

			bool converted = ilConvertImage(ilFormat.first, ilFormat.second);

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

static std::pair<ILenum, ILenum> get_image_devil_format(image_format format)
{

	ILenum formatChannels;
	ILenum formatDataType;

	switch (format)
	{

	case FUSE_IMAGE_FORMAT_R16G16B16_FLOAT:
		formatChannels = IL_RGB;
		formatDataType = IL_HALF;
		break;

	case FUSE_IMAGE_FORMAT_R32G32B32_FLOAT:
		formatChannels = IL_RGB;
		formatDataType = IL_FLOAT;
		break;

	case FUSE_IMAGE_FORMAT_R8G8B8_UINT:
		formatChannels = IL_RGB;
		formatDataType = IL_BYTE;
		break;

	case FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT:
		formatChannels = IL_RGBA;
		formatDataType = IL_HALF;
		break;

	case FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT:
		formatChannels = IL_RGBA;
		formatDataType = IL_FLOAT;
		break;

	case FUSE_IMAGE_FORMAT_R8G8B8A8_UINT:
		formatChannels = IL_RGBA;
		formatDataType = IL_BYTE;
		break;

	case FUSE_IMAGE_FORMAT_A8_UINT:
		formatChannels = IL_ALPHA;
		formatDataType = IL_BYTE;
		break;

	default:
		formatChannels = 0;
		formatDataType = 0;
		break;

	}

	return std::make_pair(formatChannels, formatDataType);

}

DXGI_FORMAT fuse::get_dxgi_format(image_format format)
{

	switch (format)
	{

	case FUSE_IMAGE_FORMAT_R16G16B16_FLOAT:
	case FUSE_IMAGE_FORMAT_R8G8B8_UINT:
	default:
		return DXGI_FORMAT_UNKNOWN;

	case FUSE_IMAGE_FORMAT_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;

	case FUSE_IMAGE_FORMAT_R16G16B16A16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;

	case FUSE_IMAGE_FORMAT_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case FUSE_IMAGE_FORMAT_R8G8B8A8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case FUSE_IMAGE_FORMAT_A8_UINT:
		return DXGI_FORMAT_A8_UNORM;

	}

}