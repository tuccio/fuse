#include <fuse/geometry/aabb.hpp>
#include <fuse/geometry/sphere.hpp>

#include <algorithm>

namespace fuse
{

	namespace detail
	{

		/* aabb/aabb */

		template <>
		struct intersection_impl<aabb, aabb> :
			intersection_base<aabb, aabb>
		{

			inline static bool FUSE_VECTOR_CALL contains(aabb a, aabb b)
			{
				vec128 aMin = a.get_min();
				vec128 aMax = a.get_max();

				vec128 bMin = b.get_min();
				vec128 bMax = b.get_max();

				vec128 andResult = vec128_and(vec128_ge(aMax, bMax), vec128_le(aMin, bMin));

				return vec128_checksign<0, 0, 0>(vec128_eq(andResult, vec128_zero()));
			}

			inline static bool FUSE_VECTOR_CALL intersects(aabb a, aabb b)
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

			inline static bool FUSE_VECTOR_CALL intersects(aabb a, sphere b)
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

			inline static bool FUSE_VECTOR_CALL intersects(aabb a, frustum b)
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

			inline static bool FUSE_VECTOR_CALL intersects(sphere a, frustum b)
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

		/* ray/aabb */

		template <>
		struct intersection_impl<ray, aabb> :
			intersection_base<ray, aabb>
		{

			inline static bool FUSE_VECTOR_CALL intersects(ray a, aabb b, float & distance)
			{
				vec128 invDirection = vec128_reciprocal(a.get_direction());

				vec128 aabbMin = b.get_min();
				vec128 aabbMax = b.get_max();

				vec128 lb = vec128_permute<FUSE_X1, FUSE_Y0, FUSE_Z0, FUSE_W0>(aabbMin, aabbMax);
				vec128 rt = vec128_permute<FUSE_X0, FUSE_Y1, FUSE_Z1, FUSE_W0>(aabbMin, aabbMax);

				vec128_f32 s0 = lb - a.get_origin() * invDirection;
				vec128_f32 s1 = rt - a.get_origin() * invDirection;

				float t1 = s0.f32[0];
				float t2 = s1.f32[0];
				float t3 = s0.f32[1];
				float t4 = s1.f32[1];
				float t5 = s0.f32[2];
				float t6 = s1.f32[2];

				float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
				float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

				if (tmax < 0)
				{
					distance = tmax;
					return false;
				}

				if (tmin > tmax)
				{
					distance = tmax;
					return false;
				}

				distance = tmin;
				return true;				
			}

			inline static bool FUSE_VECTOR_CALL intersects(ray a, aabb b)
			{
				vec128 aabbMin = b.get_min();
				vec128 aabbMax = b.get_max();

				vec128 origin = a.get_origin();

				vec128 invDirection = vec128_reciprocal(a.get_direction());

				vec128 t1 = (aabbMin - origin) * invDirection;
				vec128 t2 = (aabbMax - origin) * invDirection;

				vec128 t1x = vec128_splat<FUSE_X>(t1);
				vec128 t2x = vec128_splat<FUSE_X>(t2);

				vec128 t1y = vec128_splat<FUSE_Y>(t1);
				vec128 t2y = vec128_splat<FUSE_Y>(t2);

				vec128 tmin = vec128_min(t1x, t2x);
				vec128 tmax = vec128_max(t1x, t2x);

				tmin = vec128_max(tmin, vec128_min(t1y, t2y));
				tmax = vec128_min(tmax, vec128_max(t1y, t2y));

				return vec128_all_true(vec128_ge(tmax, tmin));
			}

		};

		/* ray/aabb */

		template <>
		struct intersection_impl<ray, sphere> :
			intersection_base<ray, sphere>
		{

			inline static bool FUSE_VECTOR_CALL intersects(ray a, sphere b, float & distance)
			{
				vec128 center = b.get_center();
				vec128 radius = b.get_radius();

				vec128 oc = a.get_origin() - center;

				// Assuming the ray direction is a normalized vector, so A = 1
				vec128 B = 2 * vec128_dot3(a.get_direction(), oc);
				vec128 C = vec128_dot3(oc, oc) - radius * radius;

				vec128 delta = B * B - 4 * C;

				if (vec128_checksign<FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE>(delta))
				{
					vec128 sqrtDelta = vec128_sqrt(delta);
					vec128 t0 = (-B - sqrtDelta) * .5f;

					if (vec128_checksign<FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE>(t0))
					{
						distance = vec128_get_x(t0);
						return true;
					}
					else
					{
						vec128 t1 = (-B + sqrtDelta) * .5f;

						if (vec128_checksign<FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE, FUSE_SIGN_POSITIVE>(t1))
						{
							distance = vec128_get_x(t1);
							return true;
						}
					}
				}
				return false;
			}

			inline static bool FUSE_VECTOR_CALL intersects(ray a, sphere b)
			{
				float t;
				return intersects(a, b, t);
			}

		};

	}

}