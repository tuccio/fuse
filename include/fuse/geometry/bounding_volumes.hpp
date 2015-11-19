#pragma once

#include <fuse/geometry/sphere.hpp>

namespace fuse
{

	/* bounding_sphere on iterable points */

	template <typename Iterator>
	sphere bounding_sphere(Iterator begin, Iterator end)
	{

		if (begin == end)
		{
			return sphere(XMVectorZero());
		}

		XMVECTOR centroid = XMVectorZero();

		for (auto it = begin; it != end; it++)
		{
			centroid += to_vector(*it);
		}

		centroid /= std::distance(begin, end);

		XMVECTOR distance = XMVectorZero();

		for (auto it = begin; it != end; it++)
		{
			distance = XMVectorMax(XMVector3Length(centroid - to_vector(*it)), distance);
		}

		return sphere(XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(centroid, distance));

	}

	/* bounding_aabb on iterable points */

	template <typename Iterator>
	aabb bounding_aabb(Iterator begin, Iterator end)
	{

		if (begin == end)
		{
			return aabb::from_center_half_extents(XMVectorZero(), XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));
		}

		XMVECTOR min = *begin;
		XMVECTOR max = *begin;

		for (auto it = begin; it != end; it++)
		{
			XMVECTOR p = to_vector(*it);
			min = XMVectorMin(min, p);
			max = XMVectorMax(max, p);
		}

		return aabb::from_min_max(min, max);

	}

	/* bounding_aabb on sphere */

	inline aabb bounding_aabb(const sphere & s)
	{

		XMVECTOR svec  = s.get_sphere_vector();
		XMVECTOR msvec = - svec;

		XMVECTOR zero = XMVectorZero();

		std::array<XMVECTOR, 6> points = {
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_1W, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(zero, svec)),
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1W, XM_PERMUTE_0Z, XM_PERMUTE_0W>(zero, svec)),
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1W, XM_PERMUTE_0W>(zero, svec)),
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_1W, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(zero, msvec)),
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1W, XM_PERMUTE_0Z, XM_PERMUTE_0W>(zero, msvec)),
			XMVectorAdd(svec, XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1W, XM_PERMUTE_0W>(zero, msvec))
		};

		return bounding_aabb(points.begin(), points.end());

	}

}