#include <fuse/logger.hpp>

#include <ctime>
#include <chrono>
#include <iomanip>

using namespace fuse;

void logger::log(const char * label, const char * message) const
{

	auto now   = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	
	*m_logStream << "[" << std::put_time(std::localtime(&now_c), "%c") << "] " << label << ": " << message << std::endl;
	
}

void logger::log(const char * label, std::ostream & os) const
{

	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);

	*m_logStream << "[" << std::put_time(std::localtime(&now_c), "%c") << "] " << label << ": " << os.rdbuf() << std::endl;

}