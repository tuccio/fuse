#pragma once

#include <fuse/math.hpp>
#include <fuse/allocators.hpp>
#include <fuse/properties_macros.hpp>

namespace fuse
{

	class alignas(16) aabb
	{

	public:

		aabb(void) = default;
		aabb(const aabb &) = default;
		aabb(aabb &&) = default;

		static aabb from_min_max(const XMVECTOR & min, const XMVECTOR & max);
		static aabb from_center_half_extents(const XMVECTOR & center, const XMVECTOR & halfExtents);

		aabb & operator= (const aabb &) = default;
		aabb & operator= (aabb &&) = default;

		inline XMVECTOR get_min(void) const { return m_center - m_halfExtents; }
		inline XMVECTOR get_max(void) const { return m_center + m_halfExtents; }

	private:

		XMVECTOR m_center;
		XMVECTOR m_halfExtents;
		
	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(center, m_center)
			(half_extents, m_halfExtents)
		)

	};

	aabb operator+ (const aabb & a, const aabb & b);
	aabb operator^ (const aabb & a, const aabb & b);

}