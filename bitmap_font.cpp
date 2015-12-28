#include <fuse/bitmap_font.hpp>
#include <fuse/image.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/gpu_render_context.hpp>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/algorithm/string.hpp>

using namespace fuse;

bool bitmap_font::load_impl(void)
{

	auto & params = get_parameters();

	auto imageExt = params.get_optional<std::string>("image_extension");
	auto metaExt = params.get_optional<std::string>("metadata_extension");

	if (!imageExt) imageExt = ".png";
	if (!metaExt) metaExt = ".xml";

	std::string name = get_name();

	std::string imageFile = name + (*imageExt);
	std::string metaFile  = name + (*metaExt);

	resource::parameters_type imageParams;

	imageParams.put<UINT>("format", static_cast<UINT>(FUSE_IMAGE_FORMAT_A8_UINT));

	resource::parameters_type textureParams;
	textureParams.put<UINT>("mipmaps", 0);
	textureParams.put<bool>("generate_mipmaps", true);

	resource_factory::get_singleton_pointer()->
		create<image>(FUSE_RESOURCE_TYPE_IMAGE, imageFile.c_str(), imageParams);

	m_texture = resource_factory::get_singleton_pointer()->
		create<texture>(FUSE_RESOURCE_TYPE_TEXTURE, imageFile.c_str(), textureParams);

	if (m_texture->load() && load_metafile(metaFile.c_str()))
	{

		ID3D12Device * device = gpu_render_context::get_singleton_pointer()->get_device();

		D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};

		srvDesc.NumDescriptors = 1;
		srvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		m_srvToken = cbv_uav_srv_descriptor_heap::get_singleton_pointer()->create_shader_resource_view(device, m_texture->get_resource());

		return true;
		
	}

	return false;

}

void bitmap_font::unload_impl(void)
{
	m_texture.reset();
}

size_t bitmap_font::calculate_size_impl(void)
{
	return m_texture->get_size();
}

bool bitmap_font::load_metafile(const char * metafile)
{

	using namespace boost::property_tree;

	iptree pt;
	std::ifstream f(metafile);
	
	if (f)
	{
		
		read_xml(f, pt);

		auto font = pt.get_child_optional("font");

		if (font)
		{

			m_height = font->get<UINT>("<xmlattr>.height");

			float invWidth  = 1.f / m_texture->get_width();
			float invHeight = 1.f / m_texture->get_height();

			for (auto character : *font)
			{

				if (boost::iequals(character.first, "char"))
				{

					auto rectString = character.second.get<std::string>("<xmlattr>.rect");
					std::stringstream ssRect(rectString);
					
					auto offsetString = character.second.get<std::string>("<xmlattr>.offset");
					std::stringstream ssOffset(offsetString);

					bitmap_char c;

					ssRect >> c.rect.left >> c.rect.top >> c.rect.right >> c.rect.bottom;
					ssOffset >> c.offset[0] >> c.offset[1];

					c.rect.right  += c.rect.left;
					c.rect.bottom += c.rect.top;

					c.code = character.second.get<char>("<xmlattr>.code");
					c.width = character.second.get<unsigned int>("<xmlattr>.width");

					c.minUV[0] = (c.rect.left) * invWidth;
					c.minUV[1] = (c.rect.top) * invHeight;

					c.maxUV[0] = (c.rect.right + .5f) * invWidth;
					c.maxUV[1] = (c.rect.bottom + .5f) * invHeight;

					m_characters[c.code] = c;

				}

			}

			return true;

		}


	}

	return false;

}

const bitmap_char & bitmap_font::operator[] (char code)
{
	return m_characters[code];
}