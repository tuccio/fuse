#pragma once

#include <mutex>

namespace fuse
{

	class lockable
	{

	public:

		using lock_type  = std::recursive_mutex;
		using guard_type = std::lock_guard<lock_type>;

		lockable(void) { }
		lockable(const lockable &) = delete;
		lockable(lockable &&) = default;

		inline void lock(void) const { m_lock.lock(); }
		inline void unlock(void) const { m_lock.unlock(); }
		inline bool try_lock(void) const { m_lock.try_lock(); }

	protected:

		mutable lock_type m_lock;

	};

}