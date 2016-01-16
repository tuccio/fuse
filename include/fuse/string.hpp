#pragma once

#include <fuse/types.hpp>

#include <string>
#include <ostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <type_traits>

namespace fuse
{

	typedef std::basic_string<char_t>  string_t;
	typedef std::basic_ostream<char_t> ostream_t;

	typedef std::basic_istringstream<char_t> istringstream_t;
	typedef std::basic_ostringstream<char_t> ostringstream_t;
	typedef std::basic_stringstream<char_t>  stringstream_t;

	template <typename CharType1, typename CharType2>
	std::enable_if_t<std::is_integral<CharType1>::value && std::is_integral<CharType2>::value, bool>
		string_equals(const CharType1 * a, const CharType2 * b)
	{

		bool equals;

		do
		{
			equals = (*(a++) == *(b++));
		} while (equals && *a && *b);

		return equals;

	}

	inline std::wstring string_widen(const std::string & s)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return std::wstring(converter.from_bytes(s));
	}

	inline std::wstring string_widen(const std::wstring & w)
	{
		return w;
	}

	inline std::string string_narrow(const std::wstring & w)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return std::string(converter.to_bytes(w));
	}

	inline std::string string_narrow(const std::string & s)
	{
		return s;
	}


#if UNICODE
	
	inline std::wstring to_string_t(int value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(long value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(long long value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(unsigned value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(unsigned long value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(unsigned long long value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(float value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(double value) { return std::to_wstring(value); }
	inline std::wstring to_string_t(long double value) { return std::to_wstring(value); }

	inline std::wstring to_string_t(const std::string & s) { return string_widen(s); }
	inline std::wstring to_string_t(const std::wstring & s) { return s; }

#else

	inline std::string to_string_t(int value) { return std::to_string(value); }
	inline std::string to_string_t(long value) { return std::to_string(value); }
	inline std::string to_string_t(long long value) { return std::to_string(value); }
	inline std::string to_string_t(unsigned value) { return std::to_string(value); }
	inline std::string to_string_t(unsigned long value) { return std::to_string(value); }
	inline std::string to_string_t(unsigned long long value) { return std::to_string(value); }
	inline std::string to_string_t(float value) { return std::to_string(value); }
	inline std::string to_string_t(double value) { return std::to_string(value); }
	inline std::string to_string_t(long double value) { return std::to_string(value); }

	inline std::string to_string_t(const std::wstring & s) { return string_narrow(s); }
	inline std::string to_string_t(const std::sstring & s) { return s; }

#endif

}