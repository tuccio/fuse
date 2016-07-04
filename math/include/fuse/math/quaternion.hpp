#pragma once

#include "vector3.hpp"
#include "vector4.hpp"

#include "vec128.hpp"

namespace fuse
{

	struct quaternion
	{

		quaternion(void) = default;
		quaternion(const quaternion &) = default;

		quaternion(const float3 & axis, float angle)
		{
			float halfAngle    = angle * .5f;

			float sinHalfAngle = std::sin(halfAngle);
			float cosHalfAngle = std::cos(halfAngle);

			x = axis.x * sinHalfAngle;
			y = axis.y * sinHalfAngle;
			z = axis.z * sinHalfAngle;
			w = cosHalfAngle;
		}

		quaternion(float w, float x, float y, float z) :
			x(x), y(y), z(z), w(w) {}

		quaternion(const vec128 & q)
		{
			auto qf32 = reinterpret_cast<const quaternion&>(q);
			w = qf32.w;
			x = qf32.x;
			y = qf32.y;
			z = qf32.z;
		}

		union
		{

			struct
			{
				float x;
				float y;
				float z;
				float w;
			};

			float m[4];

		};

	};

}