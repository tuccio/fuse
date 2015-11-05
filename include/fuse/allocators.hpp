#pragma once

#include <cstdint>
#include <memory>
#include <malloc.h>

namespace fuse
{

	template <typename T, size_t Alignment = std::alignment_of<T>::value>
	struct aligned_allocator :
		std::allocator<T>
	{

		inline void * allocate_generic(size_type n, std::allocator<void>::const_pointer hint = 0)
		{

			void * p = allocate_no_throw_generic(n, hint);

			if (!p)
			{
				throw std::bad_alloc();
			}

			return p;

		}

		inline pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
		{
			return static_cast<pointer>(allocate_generic(n, hint));
		}

		inline void * allocate_no_throw_generic(size_type n, std::allocator<void>::const_pointer hint = 0)
		{
			return _aligned_malloc(n * sizeof(T), Alignment);
		}

		inline pointer allocate_no_throw(size_type n, std::allocator<void>::const_pointer hint = 0)
		{
			return static_cast<pointer>(allocate_no_throw_generic(n, hint));
		}

		inline void deallocate(pointer p, size_type n)
		{
			deallocate_generic(static_cast<pointer>(p));
		}

		inline void deallocate_generic(void * p, size_type n)
		{
			_aligned_free(p);
		}

	};

}

#define FUSE_DECLARE_ALLOCATOR_NEW(Allocator) \
private:\
	static Allocator m_fuseAllocator;\
public:\
	inline void * operator new(size_t size) { return m_fuseAllocator.allocate_generic(size); }\
	inline void * operator new(size_t size, const std::nothrow_t &) { return m_fuseAllocator.allocate_no_throw_generic(size); }\
	inline void * operator new(size_t size, void * p){ return p; }\
	inline void operator delete(void * p) { m_fuseAllocator.deallocate_generic(p, 1); }\
	inline void operator delete(void * p, void * p2) { }

#define FUSE_DEFINE_ALLOCATOR_NEW(Class, Allocator) Allocator Class::m_fuseAllocator;

#define FUSE_ALIGNED_ALLOCATOR(Type, Alignment) fuse::aligned_allocator<Type, Alignment>
#define FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(Alignment) FUSE_DECLARE_ALLOCATOR_NEW(FUSE_ALIGNED_ALLOCATOR(uint8_t, 16))
#define FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(Class, Alignment) FUSE_DEFINE_ALLOCATOR_NEW(Class, FUSE_ALIGNED_ALLOCATOR(uint8_t, 16))