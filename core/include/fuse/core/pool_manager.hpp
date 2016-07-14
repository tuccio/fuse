#pragma once

#include <cstdint>
#include <map>

#define FUSE_POOL_INVALID ((pool_size_t) -1)

namespace fuse
{

	typedef uint32_t pool_size_t;

	class pool_manager
	{

	public:

		pool_manager(void);
		pool_manager(pool_size_t size);
		pool_manager(pool_manager &&);
		pool_manager(const pool_manager &) = delete;

		~pool_manager(void);

		pool_manager & operator= (const pool_manager & manager) = delete;
		pool_manager & operator= (pool_manager && manager);

		pool_size_t allocate(pool_size_t elements = 1u);
		void free(pool_size_t offset, pool_size_t elements = 1u);

		void clear(void);

	private:

		using map_type = std::map<pool_size_t, pool_size_t>;
		using map_iterator = map_type::iterator;

		map_type m_freeChunks;

		uint32_t m_objectsCount;

		map_iterator find_free_chunk(pool_size_t elements);
		void allocate_chunk(map_iterator it, pool_size_t elements);
		void free_chunk(pool_size_t offset, pool_size_t elements);

	};

}