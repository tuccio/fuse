#pragma once

#include <fuse/math/math.hpp>

#include <fuse/core.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/math.hpp>

namespace fuse
{

	class alignas(16) ray
	{

	public:

		ray(const vec128 & origin, const vec128 & direction) :
			m_origin(origin), m_direction(direction) { }

		ray(void) = default;
		ray(const ray &) = default;
		ray(ray &&) = default;

		vec128 evaluate(float t);

	private:

		vec128 m_origin;
		vec128 m_direction;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(direction, m_direction)
			(origin,    m_origin)
		)

	};

}