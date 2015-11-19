#pragma once

#include <fuse/resource.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/texture.hpp>
#include <fuse/properties_macros.hpp>

#include <unordered_map>

namespace fuse
{

	struct bitmap_char
	{
		char  code;
		UINT  width;
		UINT  offset[2];
		RECT  rect;
		float minUV[2];
		float maxUV[2];
	};

	class bitmap_font :
		public resource
	{

	public:

		bitmap_font(void) { }
		bitmap_font(const char * name, resource_loader * loader, resource_manager * owner) :
			resource(name, loader, owner) { }

		const bitmap_char & operator[] (char code);

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		texture_ptr m_texture;
		std::unordered_map<char, bitmap_char> m_characters;
		UINT m_height;

		com_ptr<ID3D12DescriptorHeap> m_srvHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE   m_srvDescriptor;

		bool load_metafile(const char * metafile);

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(srv_descriptor, m_srvDescriptor)
			(height, m_height)
		)

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(srv_heap, m_srvHeap)
			(texture, m_texture)
		)

	};

	typedef std::shared_ptr<bitmap_font> bitmap_font_ptr;

}