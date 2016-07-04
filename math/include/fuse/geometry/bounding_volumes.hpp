#pragma once

#include <fuse/geometry/sphere.hpp>

namespace fuse
{

	/* bounding_sphere on iterable points */

	template <typename Iterator>
	sphere bounding_sphere(Iterator begin, Iterator end)
	{
		size_t size = std::distance(begin, end);

		if (!size)
		{
			return sphere(vec128_zero());
		}

		vec128 centroid = vec128_zero();

		for (auto it = begin; it != end; it++)
		{
			vec128 t = to_vec128(*it);
			centroid += t;
		}

		centroid = centroid / (float) size;

		vec128 distance = vec128_zero();

		for (auto it = begin; it != end; ++it)
		{
			distance = vec128_max(vec128_len3(centroid - to_vec128(*it)), distance);
		}

		return sphere(centroid, distance);
	}

	/* bounding_aabb on iterable points */

	template <typename Iterator>
	aabb bounding_aabb(Iterator begin, Iterator end)
	{
		if (begin == end)
		{
			return aabb::from_center_half_extents(vec128_zero(), vec128_set(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));
		}

		vec128 min = *begin;
		vec128 max = *begin;

		for (auto it = begin; it != end; it++)
		{
			vec128 p = to_vec128(*it);
			min = vec128_min(min, p);
			max = vec128_max(max, p);
		}

		return aabb::from_min_max(min, max);
	}

	/* bounding_aabb on sphere */

	inline aabb bounding_aabb(const sphere & s)
	{
		vec128 svec  = s.get_sphere_vector();
		vec128 msvec = - svec;

		vec128 matrix_zero = XMVectorZero();

		std::array<vec128, 6> points = {
			svec + vec128_permute<FUSE_W1, FUSE_Y0, FUSE_Z0, FUSE_W0>(matrix_zero, svec),
			svec + vec128_permute<FUSE_X0, FUSE_W1, FUSE_Z0, FUSE_W0>(matrix_zero, svec),
			svec + vec128_permute<FUSE_X0, FUSE_Y0, FUSE_W1, FUSE_W0>(matrix_zero, svec),
			svec + vec128_permute<FUSE_W1, FUSE_Y0, FUSE_Z0, FUSE_W0>(matrix_zero, msvec),
			svec + vec128_permute<FUSE_X0, FUSE_W1, FUSE_Z0, FUSE_W0>(matrix_zero, msvec),
			svec + vec128_permute<FUSE_X0, FUSE_Y0, FUSE_W1, FUSE_W0>(matrix_zero, msvec)
		};

		return bounding_aabb(points.begin(), points.end());
	}

}