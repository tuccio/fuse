#include <fuse/core/logger.hpp>
#include <fuse/core/string.hpp>

#include <ctime>
#include <chrono>
#include <iomanip>

using namespace fuse;

void logger::log(const char_t * label, const char_t * message) const
{
	auto now   = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	
	*m_logStream << "[" << std::put_time(std::localtime(&now_c), L"%c") << "] " << label << ": " << message << std::endl;
}

void logger::log(const char_t * label, const std::basic_ostream<char_t> & os) const
{
	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);

	*m_logStream << "[" << std::put_time(std::localtime(&now_c), L"%c") << "] " << label << ": " << os.rdbuf() << std::endl;
}