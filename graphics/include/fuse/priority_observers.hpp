#pragma once

#include <algorithm>
#include <functional>
#include <map>

#define FUSE_PRIORITY_LOWEST  (0)
#define FUSE_PRIORITY_DEFAULT (1024)
#define FUSE_PRIORITY_HIGHEST (UINT_MAX)

#define FUSE_PRIORITY_DEFAULT_DELTA(Delta) (FUSE_PRIORITY_DEFAULT + (Delta))

namespace fuse
{


	template <typename Callback, typename Listener>
	struct priority_observable
	{

	public:

		inline void add_listener(Listener * listener, unsigned int priority = FUSE_PRIORITY_DEFAULT)
		{
			observer o;
			o.listener = listener;
			o.type = 1;
			m_observers.emplace(priority, o);
		}

		inline void add_callback(Callback callback, unsigned int priority = FUSE_PRIORITY_DEFAULT)
		{
			observer o;
			o.callback = callback;
			o.type = 0;
			m_observers.emplace(priority, o);
		}

		inline void remove_listener(Listener * listener)
		{

			for (auto it = m_observers.begin(); it != m_observers.end(); ++it)
			{
				if (((*it).second.type == 1) && ((*it).second.listener == listener))
				{
					m_observers.erase(it);
					return;
				}
			}

		}

		inline void remove_callback(Callback callback)
		{

			for (auto it = m_observers.begin(); it != m_observers.end(); ++it)
			{
				if (((*it).second.type == 0) && ((*it).second.callback == callback))
				{
					m_observers.erase(it);
					return;
				}
			}

		}

	protected:

		template <typename ... Args>
		inline void notify(bool (Listener::*listenerFunction) (Args ...), Args && ... args)
		{

			for (auto & p : m_observers)
			{

				if (p.second.type == 0)
				{
					if (p.second.callback(std::forward<Args>(args)...))
					{
						return;
					}
				}
				else
				{
					if ((p.second.listener->*listenerFunction)(std::forward<Args>(args)...))
					{
						return;
					}
				}

			}

		}

	private:

		struct observer
		{

			unsigned int type;

			union
			{
				Callback callback;
				Listener * listener;
			};

		};
		

		std::multimap<unsigned int, observer, std::greater<unsigned int>> m_observers;

	};

}