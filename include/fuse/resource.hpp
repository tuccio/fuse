#pragma once

#include <cassert>
#include <climits>
#include <functional>
#include <string>

#include "lockable.hpp"

#include <boost/property_tree/ptree.hpp>

enum resource_status
{
	FUSE_RESOURCE_NOT_LOADED,
	FUSE_RESOURCE_LOADING,
	FUSE_RESOURCE_LOADED,
	FUSE_RESOURCE_FREEING
};

namespace fuse
{

	class resource;
	class resource_manager;

	struct resource_loader
	{
		virtual bool load(resource *) = 0;
		virtual void unload(resource *) = 0;
	};

	class resource :
		lockable
	{

	public:

		using id_type         = uint32_t;
		using parameters_type = boost::property_tree::ptree;

		resource(void) :
			m_owner(nullptr), m_loader(nullptr), m_size(0), m_status(FUSE_RESOURCE_NOT_LOADED) { }

		resource(const char * name, resource_loader * loader = nullptr, resource_manager * owner = nullptr) :
			m_name(name), m_owner(owner), m_loader(loader), m_size(0), m_status(FUSE_RESOURCE_NOT_LOADED) { }

		resource(const resource &) = delete;
		resource(resource &&)      = delete;

		virtual ~resource(void) { }

		inline resource_manager * get_owner(void) const { return m_owner; }
		inline void               set_owner(resource_manager * owner) { m_owner = owner; }

		inline const char       * get_name(void) const { return m_name.c_str(); }

		inline resource_status    get_status(void) const { return m_status; }

		inline id_type            get_id(void) const { return m_id; }
		inline size_t             get_size(void) const { return m_size; }

		void                      set_parameters(const parameters_type & params) { m_parameters = params; unload(); }
		void                      set_parameters(parameters_type && params) { m_parameters = std::move(params); unload(); }
		const parameters_type   & get_parameters(void) const { return m_parameters; }

		bool load(void);
		bool lock_and_load(void);
		void unload(void);

	protected:

		virtual bool   load_impl(void) = 0;
		virtual void   unload_impl(void) = 0;
		virtual size_t calculate_size_impl(void) = 0;

		void recalculate_size(void) { m_size = calculate_size_impl(); }

		template <typename UserdataType>
		UserdataType get_owner_userdata(void) { return reinterpret_cast<UserdataType>(m_owner->get_userdata()); }

	private:

		resource_loader  * m_loader;
		resource_manager * m_owner;

		size_t             m_size;

		resource_status    m_status;
		id_type            m_id;

		std::string        m_name;
		parameters_type    m_parameters;

		friend class resource_manager;

		inline void set_id(id_type id) { m_id = id; }

	};

	inline resource::parameters_type default_parameters(void) { return resource::parameters_type(); }

	class dynamic_resource_loader :
		public resource_loader
	{

	public:

		template <typename Loader, typename Unloader>
		dynamic_resource_loader(Loader && loader, Unloader && unloader) :
			m_loader(loader),
			m_unloader(loader) { }

	protected:

		bool load(resource * r) override { assert(m_loader && m_unloader && "Cannot use dynamic_resource_loader without proper initialization"); return m_loader(r); }
		void unload(resource * r) override { assert(m_unloader && m_unloader && "Cannot use dynamic_resource_loader without proper initialization"); m_unloader(r); }

	private:

		std::function<bool(resource *)> m_loader;
		std::function<void(resource *)> m_unloader;

	};

	typedef std::shared_ptr<resource> resource_ptr;

}