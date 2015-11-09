#include <fuse/texture.hpp>
#include <fuse/resource_factory.hpp>

using namespace fuse;

bool texture::create(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_ring_buffer * ringBuffer,
	image * image,
	UINT mipmaps,
	D3D12_RESOURCE_FLAGS flags)
{

	return false;

	if (image->load())
	{

		DXGI_FORMAT format = get_dxgi_format(image->get_format());

		m_width   = image->get_width();
		m_height  = image->get_height();
		m_mipmaps = mipmaps;

		D3D12_SUBRESOURCE_DATA dataDesc = {};

		dataDesc.pData    = image->get_data();
		dataDesc.RowPitch = image->get_size();

		// TODO: Generate mips

		if (!FUSE_HR_FAILED(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(format, m_width, m_height, 1, m_mipmaps),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_buffer))))
		{

			UINT rowSize = m_width * get_dxgi_format_byte_size(format);

			D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};

			pitchedDesc.Format   = format;
			pitchedDesc.Width    = m_width;
			pitchedDesc.Height   = m_height;
			pitchedDesc.Depth    = 1;
			pitchedDesc.RowPitch = FUSE_ALIGN_OFFSET(rowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex;

			void * texdata = ringBuffer->allocate_texture(device, commandQueue, pitchedDesc, &placedTex);

			memcpy(texdata, image->get_data(), image->get_size());

		}

	}

	

}

bool texture::load_impl(void)
{

	/*image_ptr img = resource_factory::get_singleton_pointer()->
		create<image>(FUSE_RESOURCE_TYPE_IMAGE, get_name());

	if (img && img->load())
	{

		using args_tuple_type = std::tuple<ID3D12Device*, gpu_command_queue&, gpu_ring_buffer*>;

		args_tuple * args = get_owner_userdata<args_tuple*>();
		return create(std::get<0>(*args), std::get<1>(*args), std::get<2>(*args), img.get(), 1);

	}*/

	return false;
}

void texture::unload_impl(void)
{
	m_buffer.reset();
}

size_t texture::calculate_size_impl(void)
{
	
	com_ptr<ID3D12Device> device;
	
	m_buffer->GetDevice(IID_PPV_ARGS(&device));

	auto resourceDesc = m_buffer->GetDesc();
	auto allocInfo    = device->GetResourceAllocationInfo(0, 1, &resourceDesc);

	return allocInfo.SizeInBytes;

}