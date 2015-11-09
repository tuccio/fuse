#include <fuse/bitmap_font.hpp>
#include <fuse/image.hpp>

#include <fuse/resource_factory.hpp>

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

	m_texture = resource_factory::get_singleton_pointer()->
		create<texture>(FUSE_RESOURCE_TYPE_TEXTURE, imageFile.c_str());

	if (m_texture->load() && load_metafile(metaFile.c_str()))
	{

		com_ptr<ID3D12Device> device;
		m_texture->get_resource()->GetDevice(IID_PPV_ARGS(&device));

		D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};

		srvDesc.NumDescriptors = 1;
		srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		if (!FUSE_HR_FAILED(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_srvHeap))))
		{
			device->CreateShaderResourceView(m_texture->get_resource(), nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart()));
			m_srvDescriptor = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
			return true;
		}
		
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

			float width  = (float) m_texture->get_width();
			float height = (float) m_texture->get_height();

			for (auto character : *font)
			{

				if (boost::iequals(character.first, "char"))
				{

					std::stringstream ssRect(character.second.get<std::string>("<xmlattr>.rect"));

					bitmap_char c;

					ssRect >> c.rect.left >> c.rect.top >> c.rect.right >> c.rect.bottom;

					c.code = character.second.get<char>("<xmlattr>.code");
					c.width = character.second.get<unsigned int>("<xmlattr>.width");

					c.minUV[0] = c.rect.left / width;
					c.minUV[0] = c.rect.top / height;

					c.minUV[1] = c.rect.right / width;
					c.minUV[1] = c.rect.bottom / height;

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