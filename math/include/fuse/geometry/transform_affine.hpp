#pragma once

#include <fuse/geometry/sphere.hpp>

namespace fuse
{

	inline sphere transform_affine(const sphere & s, const mat128 & transform)
	{
		vec128 svec = s.get_sphere_vector();

		vec128 newCenter    = mat128_transform3(svec, transform);
		vec128 surfacePoint = vec128_permute<FUSE_W1, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_zero(), svec);

		surfacePoint = mat128_transform3(svec + surfacePoint, transform);

		vec128 distance = vec128_length3(surfacePoint - newCenter);

		return sphere(vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(newCenter, distance));
	}

}

