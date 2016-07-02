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

#include "quaternion_impl.inl"
#include "quat128_impl.inl"