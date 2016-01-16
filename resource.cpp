#include <fuse/resource.hpp>
#include <fuse/logger.hpp>

#include <sstream>

using namespace fuse;

bool resource::load(void)
{
	bool loaded = lock_and_load();
	unlock();
	return loaded;
}

bool resource::lock_and_load(void)
{

	lock();

	if (m_status == FUSE_RESOURCE_NOT_LOADED)
	{

		if ((m_loader && m_loader->load(this)) ||
			(!m_loader && load_impl()))
		{
			m_status = FUSE_RESOURCE_LOADED;
			m_size   = calculate_size_impl();
		}
		else
		{
			FUSE_LOG_OPT_DEBUG(stringstream_t() << "Failed to load resource \"" << m_name << "\".");
		}

	}

	return m_status == FUSE_RESOURCE_LOADED;

}

void resource::unload(void)
{
	
	guard_type lock(m_lock);

	if (m_status == FUSE_RESOURCE_LOADED)
	{

		if (m_loader)
		{
			m_loader->unload(this);
		}
		else
		{
			unload_impl();
		}

		m_status = FUSE_RESOURCE_NOT_LOADED;

	}

}