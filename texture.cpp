#include <fuse/texture.hpp>
#include <fuse/resource_factory.hpp>
#include <fuse/gpu_global_resource_state.hpp>

using namespace fuse;

bool texture::create(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_upload_manager * uploadManager,
	gpu_ring_buffer * ringBuffer,
	image * image,
	UINT mipmaps,
	D3D12_RESOURCE_FLAGS flags,
	bool generateMipmaps)
{

	if (image->load())
	{

		DXGI_FORMAT format = get_dxgi_format(image->get_format());

		m_width   = image->get_width();
		m_height  = image->get_height();
		m_mipmaps = mipmaps;

		D3D12_SUBRESOURCE_DATA dataDesc = {};

		dataDesc.pData    = image->get_data();
		dataDesc.RowPitch = image->get_size();

		if (generateMipmaps)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if (gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
				device,
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(format, m_width, m_height, 1, m_mipmaps, 1, 0, flags),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_buffer)))
		{

			UINT rowSize = m_width * get_dxgi_format_byte_size(format);

			D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};

			pitchedDesc.Format   = format;
			pitchedDesc.Width    = m_width;
			pitchedDesc.Height   = m_height;
			pitchedDesc.Depth    = 1;
			pitchedDesc.RowPitch = FUSE_ALIGN_OFFSET(rowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex;

			uint8_t * texdata = (uint8_t*) ringBuffer->allocate_texture(device, commandQueue, pitchedDesc, &placedTex);

			auto & img = *image;
			for (int i = 0; i < m_height; i++)
			{

				size_t offset        = i * rowSize;
				size_t alignedOffset = i * pitchedDesc.RowPitch;

				uint8_t * out = texdata + alignedOffset;
				uint8_t * in  = image->get_data() + offset;

				memcpy(out, in, rowSize);

			}

			uploadManager->upload_texture(
				commandQueue,
				&CD3DX12_TEXTURE_COPY_LOCATION(m_buffer.get(), 0),
				&CD3DX12_TEXTURE_COPY_LOCATION(ringBuffer->get_heap(), placedTex));

			if (generateMipmaps)
			{
				uploadManager->generate_mipmaps(device, commandQueue, m_buffer.get());
			}

			return true;

		}

	}

	return false;

}

void texture::clear(void)
{
	m_buffer.reset();
}

bool texture::load_impl(void)
{

	image_ptr img = resource_factory::get_singleton_pointer()->
		create<image>(FUSE_RESOURCE_TYPE_IMAGE, get_name());

	if (img && img->load())
	{

		auto & params = get_parameters();

		using args_tuple_type = std::tuple<ID3D12Device*, gpu_command_queue*, gpu_upload_manager*, gpu_ring_buffer*>;

		auto mipmaps         = params.get_optional<UINT>("mipmaps");
		auto generateMipmaps = params.get_optional<bool>("generate_mipmaps");
		auto resourceFlags   = params.get_optional<UINT>("resource_flags");

		args_tuple_type * args = get_owner_userdata<args_tuple_type*>();

		return create(
			std::get<0>(*args),
			*std::get<1>(*args),
			std::get<2>(*args),
			std::get<3>(*args),
			img.get(),
			mipmaps ? *mipmaps : 1u,
			resourceFlags ? static_cast<D3D12_RESOURCE_FLAGS>(*resourceFlags) : D3D12_RESOURCE_FLAG_NONE,
			generateMipmaps ? *generateMipmaps : false);

	}

	return false;
}

void texture::unload_impl(void)
{

	using args_tuple_type = std::tuple<ID3D12Device*, gpu_command_queue*, gpu_upload_manager*, gpu_ring_buffer*>;
	args_tuple_type * args = get_owner_userdata<args_tuple_type*>();

	std::get<1>(*args)->safe_release(m_buffer.get());

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