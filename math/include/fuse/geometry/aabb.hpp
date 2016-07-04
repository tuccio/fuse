#pragma once

#include <fuse/math/math.hpp>

#include <fuse/math.hpp>
#include <fuse/core.hpp>
#include <fuse/properties_macros.hpp>

#include <array>

#define FUSE_MAKE_AABB_CORNER(Z, Y, X) ((X) | (Y << 1) | (Z << 2))

enum aabb_corners
{
	FUSE_AABB_FRONT_BOTTOM_RIGHT = FUSE_MAKE_AABB_CORNER(0, 0, 0),
	FUSE_AABB_FRONT_BOTTOM_LEFT  = FUSE_MAKE_AABB_CORNER(0, 0, 1),
	FUSE_AABB_FRONT_TOP_RIGHT    = FUSE_MAKE_AABB_CORNER(0, 1, 0),
	FUSE_AABB_FRONT_TOP_LEFT     = FUSE_MAKE_AABB_CORNER(0, 1, 1),
	FUSE_AABB_BACK_BOTTOM_RIGHT  = FUSE_MAKE_AABB_CORNER(1, 0, 0),
	FUSE_AABB_BACK_BOTTOM_LEFT   = FUSE_MAKE_AABB_CORNER(1, 0, 1),
	FUSE_AABB_BACK_TOP_RIGHT     = FUSE_MAKE_AABB_CORNER(1, 1, 0),
	FUSE_AABB_BACK_TOP_LEFT      = FUSE_MAKE_AABB_CORNER(1, 1, 1)
};


namespace fuse
{

	class alignas(16) aabb
	{

	public:

		aabb(void) = default;
		aabb(const aabb &) = default;
		aabb(aabb &&) = default;

		static inline aabb from_min_max(const float3 & min, const float3 & max)
		{
			return from_min_max(vec128_load(min), vec128_load(max));
		}

		static inline aabb FUSE_VECTOR_CALL from_min_max(vec128 min, vec128 max)
		{
			aabb box;
			box.m_center      = (min + max) * .5f;
			box.m_halfExtents = (max - min) * .5f;
			return box;
		}

		static inline aabb from_center_half_extents(const float3 & center, const float3 & halfExtents)
		{
			return from_center_half_extents(vec128_load(center), vec128_load(halfExtents));
		}

		static inline aabb FUSE_VECTOR_CALL from_center_half_extents(vec128 center, vec128 halfExtents)
		{
			aabb box;
			box.m_center      = center;
			box.m_halfExtents = halfExtents;
			return box;
		}

		inline aabb & operator= (const aabb &) = default;
		inline aabb & operator= (aabb &&) = default;

		inline vec128 get_min(void) const { return m_center - m_halfExtents; }
		inline vec128 get_max(void) const { return m_center + m_halfExtents; }

		std::array<vec128, 8> get_corners(void) const;

	private:

		vec128 m_center;
		vec128 m_halfExtents;
		
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