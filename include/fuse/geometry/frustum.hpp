#pragma once

#include <fuse/geometry/plane.hpp>
#include <array>

#define FUSE_MAKE_FRUSTUM_CORNER(Z, Y, X) ((X) | (Y << 1) | (Z << 2))

enum frustum_corners
{
	FUSE_FRUSTUM_NEAR_BOTTOM_RIGHT = FUSE_MAKE_FRUSTUM_CORNER(0, 0, 0),
	FUSE_FRUSTUM_NEAR_BOTTOM_LEFT  = FUSE_MAKE_FRUSTUM_CORNER(0, 0, 1),
	FUSE_FRUSTUM_NEAR_TOP_RIGHT    = FUSE_MAKE_FRUSTUM_CORNER(0, 1, 0),
	FUSE_FRUSTUM_NEAR_TOP_LEFT     = FUSE_MAKE_FRUSTUM_CORNER(0, 1, 1),
	FUSE_FRUSTUM_FAR_BOTTOM_RIGHT  = FUSE_MAKE_FRUSTUM_CORNER(1, 0, 0),
	FUSE_FRUSTUM_FAR_BOTTOM_LEFT   = FUSE_MAKE_FRUSTUM_CORNER(1, 0, 1),
	FUSE_FRUSTUM_FAR_TOP_RIGHT     = FUSE_MAKE_FRUSTUM_CORNER(1, 1, 0),
	FUSE_FRUSTUM_FAR_TOP_LEFT      = FUSE_MAKE_FRUSTUM_CORNER(1, 1, 1)
};

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

		frustum(const XMMATRIX & projection, const XMMATRIX & inverseView);

		frustum & operator= (const frustum &) = default;
		frustum & operator= (frustum &&) = default;

		std::array<XMVECTOR, 8> get_corners(void) const;

	private:

		std::array<plane, 6> m_planes;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(planes, m_planes)
		)

	};

}