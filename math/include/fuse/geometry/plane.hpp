#pragma once

#include <fuse/core.hpp>
#include <fuse/math.hpp>

namespace fuse
{

	class alignas(16) plane
	{

	public:

		plane(void) = default;
		plane(const plane &) = default;
		plane(plane &&) = default;

		plane(const vec128 & plane);
		plane(float a, float b, float c, float d);
		plane(const float3 & normal, const float3 & point);
		plane(const float4 & plane);

		plane & operator= (const plane &) = default;
		plane & operator= (plane &&) = default;

		plane flip(void) const;
		plane normalize(void) const;
		vec128 FUSE_VECTOR_CALL dot(vec128 x) const;

	private:

		vec128 m_plane;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(plane_vector, m_plane)
		)

	};

}