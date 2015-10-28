#include <fuse/resource_manager.hpp>

using namespace fuse;

#define FUSE_RESOURCE_IS_UNNAMED(Name) (!name || *name == '\0')

std::shared_ptr<resource> resource_manager::create(const char * name,
	                                               const resource::parameters_type & parameters,
	                                               resource_loader * loader)
{

	guard_type lock(m_lock);

	auto hResource = find_by_name_unsafe(name);

	if (!hResource)
	{

		resource::id_type newID = ++m_lastID;

		resource * newResource = create_impl(name, loader);

		newResource->set_id(newID);

		if (!FUSE_RESOURCE_IS_UNNAMED(name))
		{
			m_namedResources[name] = newID;
		}


		hResource = std::shared_ptr<resource>(newResource);
		m_resources[newID] = hResource;

	}

	return hResource;

}

std::shared_ptr<resource> resource_manager::find_by_name(const char * name) const
{
	guard_type lock(m_lock);
	return find_by_name_unsafe(name);
}

std::shared_ptr<resource> resource_manager::find_by_id(resource::id_type id) const
{
	guard_type lock(m_lock);
	return find_by_id_unsafe(id);
}

/* Non interlocked implementations */

std::shared_ptr<resource> resource_manager::find_by_name_unsafe(const char * name) const
{

	if (!FUSE_RESOURCE_IS_UNNAMED(name))
	{

		auto it = m_namedResources.find(name);

		if (it != m_namedResources.end())
		{
			return find_by_id_unsafe(it->second);
		}

	}

	return std::shared_ptr<resource>();

}

std::shared_ptr<resource> resource_manager::find_by_id_unsafe(resource::id_type id) const
{

	auto it = m_resources.find(id);

	if (it != m_resources.end())
	{
		return it->second;
	}
	
	return std::shared_ptr<resource>();

}