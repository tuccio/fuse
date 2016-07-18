#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <fuse/core.hpp>
#include <fuse/resource.hpp>

namespace fuse
{

	class resource_manager :
		public lockable
	{

	public:

		resource_manager(const char_t * type, void * userdata = nullptr) : m_type(type), m_userdata(userdata), m_lastID(0){ }

		virtual ~resource_manager(void) { }

		std::shared_ptr<resource> create(const char_t * name,
		                                 const resource::parameters_type & parameters = resource::parameters_type(),
		                                 resource_loader * loader = nullptr);

		std::shared_ptr<resource> find_by_name(const char_t * name) const;
		std::shared_ptr<resource> find_by_id(resource::id_type id) const;

		const char_t * get_type(void) const { return m_type.c_str(); }

		void * get_userdata(void) const { return m_userdata; }

	protected:

		virtual resource * create_impl(const char_t * name, resource_loader * loader) = 0;
		virtual void       free_impl(resource * resource) = 0;

	private:

		void              * m_userdata;
		resource::id_type   m_lastID;
		string_t            m_type;

		std::unordered_map<string_t, resource::id_type>                  m_namedResources;
		std::unordered_map<resource::id_type, std::shared_ptr<resource>> m_resources;

		/* Non interlocked implementations */

		std::shared_ptr<resource> find_by_name_unsafe(const char_t * name) const;
		std::shared_ptr<resource> find_by_id_unsafe(resource::id_type id)    const;

	};

}