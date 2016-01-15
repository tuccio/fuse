#pragma once

#include <fuse/allocators.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/math.hpp>

namespace fuse
{

	class alignas(16) ray
	{

	public:

		ray(const XMVECTOR & origin, const XMVECTOR & direction) :
			m_origin(origin), m_direction(direction) { }

		ray(void) = default;
		ray(const ray &) = default;
		ray(ray &&) = default;

		XMVECTOR evaluate(float t);

	private:

		XMVECTOR m_origin;
		XMVECTOR m_direction;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(direction, m_direction)
			(origin,    m_origin)
		)

	};

}