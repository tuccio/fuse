#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>
#include <fuse/properties_macros.hpp>

namespace fuse
{

	class alignas(16) plane
	{

	public:

		plane(void) = default;
		plane(const plane &) = default;
		plane(plane &&) = default;

		plane(const XMVECTOR & plane);
		plane(float a, float b, float c, float d);
		plane(const XMFLOAT3 & normal, const XMFLOAT3 & point);

		plane & operator= (const plane &) = default;
		plane & operator= (plane &&) = default;

		plane flip(void) const;
		plane normalize(void) const;

	private:

		XMVECTOR m_plane;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(plane_vector, m_plane)
		)

	};

}