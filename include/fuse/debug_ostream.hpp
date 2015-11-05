#pragma once

#include <ostream>

#include <Windows.h>

namespace fuse
{

	class debug_streambuf :
		public std::streambuf
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
		public std::ostream
	{

	public:

		debug_ostream(void) : std::ostream(&m_streambuf) { }

	private:

		debug_streambuf m_streambuf;

	};

}