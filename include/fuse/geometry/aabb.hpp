#pragma once

#include <fuse/math.hpp>
#include <fuse/allocators.hpp>
#include <fuse/properties_macros.hpp>

#include <array>

#define FUSE_MAKE_CORNER(Z, Y, X) ((X) | (Y << 1) | (Z << 2))

enum aabb_corners
{
	FUSE_AABB_FRONT_BOTTOM_RIGHT = FUSE_MAKE_CORNER(0, 0, 0),
	FUSE_AABB_FRONT_BOTTOM_LEFT  = FUSE_MAKE_CORNER(0, 0, 1),
	FUSE_AABB_FRONT_TOP_RIGHT    = FUSE_MAKE_CORNER(0, 1, 0),
	FUSE_AABB_FRONT_TOP_LEFT     = FUSE_MAKE_CORNER(0, 1, 1),
	FUSE_AABB_BACK_BOTTOM_RIGHT  = FUSE_MAKE_CORNER(1, 0, 0),
	FUSE_AABB_BACK_BOTTOM_LEFT   = FUSE_MAKE_CORNER(1, 0, 1),
	FUSE_AABB_BACK_TOP_RIGHT     = FUSE_MAKE_CORNER(1, 1, 0),
	FUSE_AABB_BACK_TOP_LEFT      = FUSE_MAKE_CORNER(1, 1, 1),
};


namespace fuse
{

	class alignas(16) aabb
	{

	public:

		aabb(void) = default;
		aabb(const aabb &) = default;
		aabb(aabb &&) = default;

		static inline aabb from_min_max(const XMVECTOR & min, const XMVECTOR & max)
		{
			aabb box;
			box.m_center      = XMVectorAdd(min, max) * .5f;
			box.m_halfExtents = XMVectorSubtract(max, min) * .5f;
			return box;
		}

		static inline aabb from_center_half_extents(const XMVECTOR & center, const XMVECTOR & halfExtents)
		{
			aabb box;
			box.m_center      = center;
			box.m_halfExtents = halfExtents;
			return box;
		}

		inline aabb & operator= (const aabb &) = default;
		inline aabb & operator= (aabb &&) = default;

		inline XMVECTOR get_min(void) const { return XMVectorSubtract(m_center, m_halfExtents); }
		inline XMVECTOR get_max(void) const { return XMVectorAdd(m_center, m_halfExtents); }

		std::array<XMVECTOR, 8> get_corners(void) const;

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