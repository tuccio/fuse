#pragma once

#include <string>
#include <unordered_map>

#include <fuse/logger.hpp>
#include <fuse/singleton.hpp>

#include <fuse/resource_manager.hpp>
#include <fuse/resource_types.hpp>

namespace fuse
{

	class resource_factory :
		public singleton<resource_factory>
	{

	public:

		resource_factory(void) = default;
		~resource_factory(void);

		void clear(void);
		
		void register_manager(resource_manager * manager);
		void unregister_manager(const char_t * type);

		template <typename ResourceType = resource>
		std::shared_ptr<ResourceType> create(const char_t * type,
		                                     const char_t * name,
		                                     const resource::parameters_type & parameters = resource::parameters_type(),
		                                     resource_loader * loader = nullptr)
		{

			auto it = m_managers.find(type);

			if (it != m_managers.end())
			{
				return std::static_pointer_cast<ResourceType>(it->second->create(name, parameters, loader));
			}
			else
			{
				FUSE_LOG_OPT_DEBUG(stringstream_t() << "Requested resource type (" << type << ") is unregistered.");
				return std::shared_ptr<ResourceType>();
			}

		}

	private:

		std::unordered_map<string_t, resource_manager *> m_managers;

	};

}