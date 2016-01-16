#pragma once

#include <ostream>

#include <Windows.h>

namespace fuse
{

	class debug_streambuf :
		public std::basic_streambuf<TCHAR>
	{

		std::streamsize xsputn(const char_type * s, std::streamsize n) override
		{
			OutputDebugString(s);
			return n;
		}

		int_type overflow(int_type c) override
		{
			TCHAR s[] = { c, '\0' };
			OutputDebugString(s);
			return 0;
		}

	};

	class debug_ostream :
		public std::basic_ostream<TCHAR>
	{

	public:

		debug_ostream(void) : std::basic_ostream<TCHAR>(&m_streambuf) { }

	private:

		debug_streambuf m_streambuf;

	};

}