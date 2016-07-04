#include <fuse/image.hpp>
#include <fuse/core.hpp>

#include <IL/il.h>
#include <IL/ilu.h>

#include <utility>

using namespace fuse;

image::devil_initializer::devil_initializer(void) { ilInit(); }
image::devil_initializer::~devil_initializer(void) { ilShutDown(); }

image::devil_initializer image::m_initializer;

static std::pair<ILenum, ILenum> get_image_devil_format(image_format format);

struct scoped_devil_handle
{

	ILuint handle;

	operator ILuint () { return handle; }

	scoped_devil_handle(void) : handle(IL_INVALID_VALUE) { }
	scoped_devil_handle(ILuint handle) : handle(handle) { }

	~scoped_devil_handle(void)
	{
		if (handle != IL_INVALID_VALUE)
		{
			ilDeleteImage(handle);
		}
	}

	scoped_devil_handle & operator= (ILuint handle) { this->handle = handle; }

};

image::image(const char_t * name, resource_loader * loader, resource_manager * owner) :
	resource(name, loader, owner) { }

image::~image(void)
{
	clear();
}

bool image::load_impl(void)
{

	bool loaded = false;

	clear();

	scoped_devil_handle handle = ilGenImage();

	if (handle != IL_INVALID_VALUE)
	{

		ilBindImage(handle);

		if (ilLoadImage(get_name()))
		{

			auto format = get_parameters().get_optional<int>(FUSE_LITERAL("format"));

			if (!format)
			{
				format = FUSE_IMAGE_FORMAT_R8G8B8A8_UINT;
			}

			m_format = static_cast<image_format>(*format);

			auto ilFormat = get_image_devil_format(m_format);

			if (!ilFormat.first)
			{
				ilFormat.first  = IL_RGBA;
				ilFormat.second = IL_UNSIGNED_BYTE;
				m_format        = FUSE_IMAGE_FORMAT_R8G8B8A8_UINT;
			}

			if (ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT)
			{
				iluFlipImage();
			}

			m_width  = ilGetInteger(IL_IMAGE_WIDTH);
			m_height = ilGetInteger(IL_IMAGE_HEIGHT);

			size_t size = get_dxgi_format_byte_size(get_dxgi_format(m_format));

			m_data.resize(m_width * m_height * size);

			ILuint copyResult = ilCopyPixels(0, 0, 0, m_width, m_height, 1, ilFormat.first, ilFormat.second, &m_data[0]);
			loaded = (m_data.size() == copyResult);

		}
		else
		{
			FUSE_LOG_OPT_DEBUG(stringstream_t() << "Unable to load image file \"" << get_name() << "\".");
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
	return m_data.size();
}

void image::clear(void)
{
	m_data.clear();
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
		formatDataType = IL_UNSIGNED_BYTE;
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
		formatDataType = IL_UNSIGNED_BYTE;
		break;

	case FUSE_IMAGE_FORMAT_A8_UINT:
		formatChannels = IL_ALPHA;
		formatDataType = IL_UNSIGNED_BYTE;
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

bool image::create(image_format format, uint32_t width, uint32_t height, const void * data)
{

	size_t size = width * height * get_dxgi_format_byte_size(get_dxgi_format(format));

	m_data.resize(size);

	m_width = width;
	m_height = height;

	m_format = format;

	memcpy(&m_data[0], data, size);

	return true;

}