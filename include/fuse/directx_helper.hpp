#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <fuse/logger.hpp>

#include <cassert>
#include <ostream>
#include <string>

#include <d3dx12.h>

#define FUSE_BLOB_ARGS(Blob) Blob->GetBufferPointer(), Blob->GetBufferSize()
#define FUSE_BLOB_LOG(Blob) { const char * msg = static_cast<const char *>(Blob->GetBufferPointer()); FUSE_LOG_OPT_DEBUG(msg); }

#define FUSE_HR_LOG(HR)\
		{\
			LPTSTR output;\
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,\
			nullptr,\
			HR,\
			MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),\
			(LPTSTR) &output,\
			0x0,\
			nullptr);\
			FUSE_LOG_OPT_DEBUG(output);\
			LocalFree((HLOCAL) output);\
		}

#define FUSE_HR_CHECK(HR)  { HRESULT hr = HR; if (FAILED(hr)) FUSE_HR_LOG(hr); }
#define FUSE_HR_FAILED(HR) ([] (HRESULT hr) { if (FAILED(hr)) { FUSE_HR_LOG(hr); return true; } return false; } (HR))

#define FUSE_HR_CHECK_BLOB(HR, Blob) { HRESULT hr = HR; if (FAILED(hr)) FUSE_BLOB_LOG(Blob) }
#define FUSE_HR_FAILED_BLOB(HR, Blob) ([&Blob] (HRESULT hr) { if (FAILED(hr)) { FUSE_BLOB_LOG(Blob); return true; } return false; } (HR))

namespace fuse
{

	template <typename ComObject>
	class com_ptr
	{

	public:

		inline com_ptr(void) : m_pointer(nullptr) { }

		inline com_ptr(ComObject * pointer) : m_pointer(pointer) { if (m_pointer) m_pointer->AddRef(); }

		inline com_ptr(const com_ptr<ComObject> & com) : com_ptr(com.get()) { }
		inline com_ptr(com_ptr<ComObject> && com) { m_pointer = com.m_pointer; com.m_pointer = nullptr; }

		inline ~com_ptr(void) { if (m_pointer) m_pointer->Release(); }

		inline ComObject * get(void) const { return m_pointer; }
														      
		inline ComObject * const * get_address(void) const { return &m_pointer; }
		inline ComObject **        get_address(void)       { return &m_pointer; }
														      
		inline void        reset(void) { if (m_pointer) { m_pointer->Release(); m_pointer = nullptr; } }
		inline ComObject * release(void) { ComObject * ptr = m_pointer; m_pointer = nullptr; return ptr; }

		inline void swap(com_ptr<ComObject> & com) { std::swap(m_pointer, com.m_pointer); }

		inline operator bool() const { return m_pointer != nullptr; }

		inline ComObject * operator-> (void) const { return m_pointer;  }

		inline ComObject * const * operator&(void) const { return &m_pointer; }
		inline ComObject **        operator&(void) { return &m_pointer; }

		inline bool operator == (const com_ptr<ComObject> & com) { return m_pointer == com.m_pointer; }
		inline bool operator != (const com_ptr<ComObject> & com) { return m_pointer != com.m_pointer; }

		inline com_ptr & operator= (const com_ptr<ComObject> & com) { reset(); m_pointer = com.m_pointer; m_pointer->AddRef(); return *this; }
		inline com_ptr & operator= (com_ptr<ComObject> && com)      { reset(); swap(com); return *this; }

		template <typename Interface>
		inline void as(com_ptr<Interface> & com) { com.reset(); m_pointer->QueryInterface(IID_PPV_ARGS(com.get_address())); }

	private:

		ComObject * m_pointer;

	};

	inline size_t get_dxgi_format_byte_size(DXGI_FORMAT format)
	{

		// TODO

		switch (format)
		{

		case DXGI_FORMAT_A8_UNORM:
			return 1;

		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R32_UINT:
			return 4;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return 8;

		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return 16;

		default:
			assert(false && "get_dxgi_format_size unknown type");
			return 0;
		}

	}

	inline D3D12_VIEWPORT make_fullscreen_viewport(float width, float height)
	{

		D3D12_VIEWPORT viewport = {};

		viewport.Height   = height;
		viewport.Width    = width;
		viewport.MaxDepth = 1.f;

		return viewport;

	}

	inline D3D12_RECT make_fullscreen_scissor_rect(UINT width, UINT height)
	{

		D3D12_RECT scissorRect = {};

		scissorRect.right  = width;
		scissorRect.bottom = height;

		return scissorRect;

	}

}