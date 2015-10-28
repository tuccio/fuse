#pragma once

#include <fuse/singleton.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <ostream>

#define FUSE_LOG_LABEL               BOOST_PP_CAT(BOOST_PP_CAT(__FUNCTION__, " @ "), BOOST_PP_CAT(BOOST_PP_CAT(__FILE__, ":"), BOOST_PP_CAT(, BOOST_PP_STRINGIZE(__LINE__))))
#define FUSE_LOG(Label, Message)     fuse::logger::get_singleton_reference().log(Label, Message);
#define FUSE_LOG_OPT(Label, Message) { auto pLogger = fuse::logger::get_singleton_pointer(); if (pLogger) pLogger->log(Label, Message); }
#define FUSE_LOG_DEBUG(Message)      FUSE_LOG(FUSE_LOG_LABEL, Message)
#define FUSE_LOG_OPT_DEBUG(Message)  FUSE_LOG_OPT(FUSE_LOG_LABEL, Message)

namespace fuse
{

	class logger :
		public singleton<logger>
	{

	public:

		logger(std::ostream * stream) {	m_logStream = stream; }

		void log(const char * label, const char * message) const;
		void log(const char * label, std::ostream & stream) const;

	private:

		std::ostream * m_logStream;

	};

}