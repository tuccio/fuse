#pragma once

#include <fuse/resource.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/texture.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/descriptor_heap.hpp>

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
		bitmap_font(const char_t * name, resource_loader * loader, resource_manager * owner) :
			resource(name, loader, owner) { }

		const bitmap_char & operator[] (char_t code);

		inline D3D12_GPU_DESCRIPTOR_HANDLE get_srv_descriptor(void) const { return cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(m_srvToken); }

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		texture_ptr m_texture;
		std::unordered_map<char_t, bitmap_char> m_characters;
		UINT m_height;

		descriptor_token_t m_srvToken;

		bool load_metafile(const char_t * metafile);

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(height, m_height)
		)

		FUSE_PROPERTIES_SMART_POINTER_READ_ONLY(
			(texture, m_texture)
		)

	};

	typedef std::shared_ptr<bitmap_font> bitmap_font_ptr;

}