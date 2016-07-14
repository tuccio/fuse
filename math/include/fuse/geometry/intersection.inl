#include <fuse/geometry/aabb.hpp>
#include <fuse/geometry/sphere.hpp>

namespace fuse
{

	namespace detail
	{

		/* aabb/aabb */

		template <>
		struct intersection_impl<aabb, aabb> :
			intersection_base<aabb, aabb>
		{

			inline static bool contains(const aabb & a, const aabb & b)
			{
				vec128 aMin = a.get_min();
				vec128 aMax = a.get_max();

				vec128 bMin = b.get_min();
				vec128 bMax = b.get_max();

				vec128 andResult = vec128_and(vec128_ge(aMax, bMax), vec128_le(aMin, bMin));

				return vec128_checksign<0, 0, 0>(vec128_eq(andResult, vec128_zero()));
			}

			inline static bool intersects(const aabb & a, const aabb & b)
			{
				vec128 aCenter      = a.get_center();
				vec128 aHalfExtents = a.get_half_extents();

				vec128 bCenter      = b.get_center();
				vec128 bHalfExtents = b.get_half_extents();

				vec128 t1 = aCenter - bCenter;
				vec128 t2 = aHalfExtents + bHalfExtents;

				return vec128_checksign<1, 1, 1>(vec128_gt(t2, t1));
			}

		};

		/* aabb/sphere */

		template <>
		struct intersection_impl<aabb, sphere> :
			intersection_base<aabb, sphere>
		{

			inline static bool intersects(const aabb & a, const sphere & b)
			{
				vec128 aabbMin = a.get_min();
				vec128 aabbMax = a.get_max();

				vec128 sphereCenter = b.get_center();
				vec128 sphereRadius = b.get_radius();

				vec128 d = vec128_zero();

				vec128 t1 = vec128_saturate(aabbMin - sphereCenter);
				vec128 t2 = vec128_saturate(sphereCenter - aabbMax);

				vec128 e = t1 + t2;

				d = e * e + d;

				vec128 r2 = sphereRadius * sphereRadius;

				d = vec128_splat<FUSE_X>(d) + vec128_splat<FUSE_Y>(d) + vec128_splat<FUSE_Z>(d);

				return vec128_checksign<1, 1, 1>(vec128_gt(e, sphereRadius)) &&
				       !vec128_checksign<0, 0, 0>(vec128_gt(d, r2));
			}

		};

		/* aabb/frustum */

		template <>
		struct intersection_impl<aabb, frustum> :
			intersection_base<aabb, frustum>
		{

			inline static bool intersects(const aabb & a, const frustum & b)
			{
				auto planes = b.get_planes();

				float3 min = vec128_f32(a.get_min()).v3f;
				float3 max = vec128_f32(a.get_max()).v3f;

				for (int i = 0; i < 6; i++)
				{

					vec128 planeVector = planes[i].get_plane_vector();

					float3 normal = vec128_f32(planeVector).v3f;
					float3 n, p;

					if (normal.x > 0)
					{
						n.x = min.x;
						p.x = max.x;
					}
					else
					{
						n.x = max.x;
						p.x = min.x;
					}

					if (normal.y > 0)
					{
						n.y = min.y;
						p.y = max.y;
					}
					else
					{
						n.y = max.y;
						p.y = min.y;
					}

					if (normal.z > 0)
					{
						n.z = min.z;
						p.z = max.z;
					}
					else
					{
						n.z = max.z;
						p.z = min.z;
					}

					//if (vec128_checksign<1, 1, 1, 1>(vec128_dp4(planeVector, vec128_load(n))))
					//if (vec128GetX(XMPlaneDotCoord(planeVector, vec128_load(n))) > 0)
					//if (vec128_get_x(vec128_dp4(planeVector, vec128_load(n))) > 0)
					if (vec128_get_x(plane(planeVector).dot(vec128_load(n))) > 0)
					{
						return false;
					}

				}

				return true;
			}

		};

		/* sphere/frustum */

		template <>
		struct intersection_impl<sphere, frustum> :
			intersection_base<sphere, frustum>
		{

			inline static bool intersects(const sphere & a, const frustum & b)
			{

				// http://www.flipcode.com/archives/Frustum_Culling.shtml

				auto planes = b.get_planes();

				vec128 sphereCenter = a.get_center();
				vec128 sphereRadius = a.get_radius();

				for (int i = 0; i < 6; i++)
				{

					vec128 planeVector = planes[i].get_plane_vector();

					// Sphere-Frustum distance
					//float distance = vec128GetX(XMPlaneDotCoord(planeVector, sphereCenter));
					float distance = vec128_get_x(plane(planeVector).dot(sphereCenter));

					if (distance > vec128_get_x(sphereRadius))
					{
						return false;
					}

				}

				return true;

			}

		};

	}

}