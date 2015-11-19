#pragma once

#include <fuse/geometry/plane.hpp>
#include <array>

enum frustum_planes
{
	FUSE_FRUSTUM_PLANE_NEAR,
	FUSE_FRUSTUM_PLANE_FAR,
	FUSE_FRUSTUM_PLANE_TOP,
	FUSE_FRUSTUM_PLANE_BOTTOM,
	FUSE_FRUSTUM_PLANE_LEFT,
	FUSE_FRUSTUM_PLANE_RIGHT
};

namespace fuse
{

	class alignas(16) frustum
	{

	public:

		frustum(void) = default;
		frustum(const frustum &) = default;
		frustum(frustum &&) = default;

		frustum(const XMMATRIX & viewProjection);

		frustum & operator= (const frustum &) = default;
		frustum & operator= (frustum &&) = default;

	private:

		std::array<plane, 6> m_planes;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(planes, m_planes)
		)

	};

}