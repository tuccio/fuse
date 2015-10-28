#pragma once

#include <Windows.h>

#include <fuse/logger.hpp>

#include <ostream>
#include <string>

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

		inline void swap(com_ptr<ComObject*> & com) { std::swap(m_pointer, com.m_pointer); }

		inline operator bool() const { return m_pointer != nullptr; }

		inline ComObject * operator-> (void) const { return m_pointer;  }

		inline ComObject * const * operator&(void) const { return &m_pointer; }
		inline ComObject **        operator&(void) { return &m_pointer; }

		inline bool operator == (const com_ptr<ComObject*> & com) { return m_pointer == com.m_pointer; }
		inline bool operator != (const com_ptr<ComObject*> & com) { return m_pointer != com.m_pointer; }

		inline com_ptr & operator= (const com_ptr<ComObject*> & com) { reset(); m_pointer = com.m_pointer; m_pointer->AddRef(); return *this; }
		inline com_ptr & operator= (com_ptr<ComObject*> && com)      { reset(); swap(com); return *this; }

		template <typename Interface>
		inline void as(com_ptr<Interface> & com) { com.reset(); m_pointer->QueryInterface(IID_PPV_ARGS(com.get_address())); }

	private:

		ComObject * m_pointer;

	};

}