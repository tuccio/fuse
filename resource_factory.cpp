#include <fuse/resource_factory.hpp>

using namespace fuse;

resource_factory::~resource_factory(void)
{
	clear();
}

void resource_factory::clear(void)
{
	m_managers.clear();
}

void resource_factory::register_manager(resource_manager * manager)
{
	m_managers[manager->get_type()] = manager;
}

void resource_factory::unregister_manager(const char * type)
{
	
	auto it = m_managers.find(type);

	if (it != m_managers.end())
	{
		m_managers.erase(it);
	}

}