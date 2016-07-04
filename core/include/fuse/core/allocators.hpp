#pragma once

#include <cstdint>
#include <memory>
#include <malloc.h>

namespace fuse
{

	template <typename T>
	struct allocator_base
	{

		typedef T value_type;
		typedef T * pointer;
		typedef const T * const_pointer;
		typedef T & reference;
		typedef const T & const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		template <typename ... Args>
		void construct(T * p, Args && ... args)
		{
			new (p) T(args ...);
		}

		void destroy(T * p)
		{
			p->~T();
		}

	};

	template <typename T, size_t Alignment = std::alignment_of<T>::value>
	struct aligned_allocator :
		allocator_base<T>
	{

		template <class U> struct rebind { typedef aligned_allocator<U, Alignment> other; };

		aligned_allocator(void) = default;
		aligned_allocator(const aligned_allocator &) = default;
		aligned_allocator(aligned_allocator &&) = default;

		template <typename U>
		aligned_allocator(const aligned_allocator<U> &) { }

		inline void * allocate_generic(size_type n, const void * hint = 0)
		{

			void * p = allocate_no_throw_generic(n, hint);

			if (!p)
			{
				throw std::bad_alloc();
			}

			return p;

		}

		inline pointer allocate(size_type n, const void * hint = 0)
		{
			return static_cast<pointer>(allocate_generic(n, hint));
		}

		inline void * allocate_no_throw_generic(size_type n, const void * hint = 0)
		{
			return _aligned_malloc(n * sizeof(T), Alignment);
		}

		inline pointer allocate_no_throw(size_type n, const void * hint = 0)
		{
			return static_cast<pointer>(allocate_no_throw_generic(n, hint));
		}

		inline void deallocate(pointer p, size_type n = 1)
		{
			deallocate_generic(static_cast<pointer>(p), n);
		}

		inline void deallocate_generic(void * p, size_type n)
		{
			_aligned_free(p);
		}

		template<class U>
		inline aligned_allocator<T, Alignment> & operator= (const aligned_allocator<U, Alignment> &)
		{
			return (*this);
		}

		template <typename U, size_t OtherAlignment>
		inline bool operator== (const aligned_allocator<U, OtherAlignment> &) { return OtherAlignment == Alignment; }

		template <typename U, size_t OtherAlignment>
		inline bool operator!= (const aligned_allocator<U, OtherAlignment> &) { return OtherAlignment != Alignment; }

	};

}

#define FUSE_ALLOCATOR_STATIC_MEMBER m_staticFuseAllocator

#define FUSE_DECLARE_ALLOCATOR_NEW(Allocator) \
private:\
	static Allocator FUSE_ALLOCATOR_STATIC_MEMBER;\
public:\
	inline void * operator new(size_t size) { return FUSE_ALLOCATOR_STATIC_MEMBER.allocate_generic(size); }\
	inline void * operator new(size_t size, const std::nothrow_t &) { return FUSE_ALLOCATOR_STATIC_MEMBER.allocate_no_throw_generic(size); }\
	inline void * operator new(size_t size, void * p){ return p; }\
	inline void operator delete(void * p) { FUSE_ALLOCATOR_STATIC_MEMBER.deallocate_generic(p, 1); }\
	inline void operator delete(void * p, void * p2) { }

#define FUSE_DEFINE_ALLOCATOR_NEW(Class, Allocator) Allocator Class::FUSE_ALLOCATOR_STATIC_MEMBER;

#define FUSE_ALIGNED_ALLOCATOR(Type, Alignment) fuse::aligned_allocator<Type, Alignment>
#define FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(Alignment) FUSE_DECLARE_ALLOCATOR_NEW(FUSE_ALIGNED_ALLOCATOR(uint8_t, Alignment))
#define FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(Class, Alignment) FUSE_DEFINE_ALLOCATOR_NEW(Class, FUSE_ALIGNED_ALLOCATOR(uint8_t, Alignment))