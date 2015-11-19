#pragma once

#include <fuse/geometry/sphere.hpp>

namespace fuse
{

	inline sphere transform_affine(const sphere & s, const XMMATRIX & transform)
	{

		XMVECTOR svec = s.get_sphere_vector();

		XMVECTOR newCenter = XMVector3Transform(svec, transform);
		XMVECTOR surfacePoint = XMVectorPermute<XM_PERMUTE_1W, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(XMVectorZero(), svec);

		XMVECTOR distance = XMVector3Length(surfacePoint - newCenter);

		return sphere(XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(newCenter, distance));

	}

}

