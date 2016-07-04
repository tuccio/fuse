#pragma once

#include <cassert>

namespace fuse
{

	template <typename T>
	class singleton
	{

	public:

		singleton(void)
		{
			assert(!s_singleton && "Cannot create two instances of a singleton class.");
			s_singleton = static_cast<T*>(this);
		}

		singleton(const singleton &) = delete;
		singleton(singleton &&) = default;

		~singleton(void)
		{
			s_singleton = nullptr;
		}

		inline static T * get_singleton_pointer(void) { return s_singleton; }
		inline static T & get_singleton_reference(void) { return *s_singleton; }

	private:

		static T * s_singleton;

	};

	template <typename T>
	T * singleton<T>::s_singleton;

}