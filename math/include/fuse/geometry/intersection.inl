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

				XMVECTOR aMin = a.get_min();
				XMVECTOR aMax = a.get_max();

				XMVECTOR bMin = b.get_min();
				XMVECTOR bMax = b.get_max();

				XMVECTOR andResult = XMVectorAndInt(XMVectorGreaterOrEqual(aMax, bMax), XMVectorLessOrEqual(aMin, bMin));
				
				return XMComparisonAllFalse(XMVector3EqualR(andResult, XMVectorZero()));

			}

			inline static bool intersects(const aabb & a, const aabb & b)
			{

				XMVECTOR aCenter      = a.get_center();
				XMVECTOR aHalfExtents = a.get_half_extents();

				XMVECTOR bCenter      = b.get_center();
				XMVECTOR bHalfExtents = b.get_half_extents();

				XMVECTOR t1 = XMVectorSubtract(aCenter, bCenter);
				XMVECTOR t2 = XMVectorAdd(aHalfExtents, bHalfExtents);

				return XMComparisonAllTrue(XMVector3Greater(t2, t1));

			}

		};

		/* aabb/sphere */

		template <>
		struct intersection_impl<aabb, sphere> :
			intersection_base<aabb, sphere>
		{

			inline static bool intersects(const aabb & a, const sphere & b)
			{

				XMVECTOR aabbMin = a.get_min();
				XMVECTOR aabbMax = a.get_max();

				XMVECTOR sphereCenter = b.get_center();
				XMVECTOR sphereRadius = b.get_radius();

				XMVECTOR d = XMVectorZero();

				XMVECTOR t1 = XMVectorSaturate(XMVectorSubtract(aabbMin, sphereCenter));
				XMVECTOR t2 = XMVectorSaturate(XMVectorSubtract(sphereCenter, aabbMax));

				XMVECTOR e = XMVectorAdd(t1, t2);

				d = XMVectorMultiplyAdd(e, e, d);

				XMVECTOR r2 = XMVectorMultiply(sphereRadius, sphereRadius);

				d = XMVectorAdd(XMVectorAdd(XMVectorSplatX(d), XMVectorSplatY(d)), XMVectorSplatZ(d));

				return XMComparisonAllTrue(XMVector3Greater(e, sphereRadius)) &&
				       XMComparisonAnyTrue(XMVector3Greater(d, r2));

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

				XMFLOAT3 min = to_float3(a.get_min());
				XMFLOAT3 max = to_float3(a.get_max());

				for (int i = 0; i < 6; i++)
				{

					XMVECTOR planeVector = planes[i].get_plane_vector();

					XMFLOAT3 normal = to_float3(planeVector);
					XMFLOAT3 n, p;

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

					if (XMVectorGetX(XMPlaneDotCoord(planeVector, to_vector(n))) > 0)
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

				XMVECTOR sphereCenter = a.get_center();
				XMVECTOR sphereRadius = a.get_radius();

				for (int i = 0; i < 6; i++)
				{

					XMVECTOR planeVector = planes[i].get_plane_vector();

					// Sphere-Frustum distance
					float distance = XMVectorGetX(XMPlaneDotCoord(planeVector, sphereCenter));

					if (distance > sphereRadius.m128_f32[0])
					{
						return false;
					}

				}

				return true;

			}

		};

	}

}