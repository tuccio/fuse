#pragma once

#include <fuse/math/math.hpp>

#include <fuse/core.hpp>
#include <fuse/math.hpp>
#include <fuse/properties_macros.hpp>

#include <type_traits>

namespace fuse
{

	class alignas(16) sphere
	{

	public:

		sphere(void) = default;
		sphere(const sphere &) = default;
		sphere(sphere &&) = default;

		sphere(const float3 & center, float radius);
		sphere(const vec128 & center, const vec128 & radius);
		sphere(const vec128 & sphere);

		void FUSE_VECTOR_CALL set_center(vec128 center);

		vec128 get_radius(void) const;
		void FUSE_VECTOR_CALL set_radius(vec128 radius);

		sphere & operator= (const sphere &) = default;
		sphere & operator= (sphere &&) = default;

	private:

		vec128 m_sphere;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(sphere_vector, m_sphere)
		)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(center, m_sphere)
		)

	};

}