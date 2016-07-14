#pragma once

#include "vec128.hpp"
#include "matrix4x4.hpp"

namespace fuse
{

	struct alignas(16) mat128
	{
		vec128 c[4];
	};

	struct alignas(16) mat128_f32
	{

		inline mat128_f32(void) = default;

		inline mat128_f32(const mat128 & rhs)
		{
			c[0] = rhs.c[0];
			c[1] = rhs.c[1];
			c[2] = rhs.c[2];
			c[3] = rhs.c[3];
		}

		inline mat128_f32(const mat128_f32 & rhs)
		{
			c[0] = rhs.c[0];
			c[1] = rhs.c[1];
			c[2] = rhs.c[2];
			c[3] = rhs.c[3];
		}

		inline mat128_f32(mat128_f32 && rhs)
		{
			c[0] = rhs.c[0];
			c[1] = rhs.c[1];
			c[2] = rhs.c[2];
			c[3] = rhs.c[3];
		}

		inline mat128_f32(float d)
		{
			c[0] = vec128_f32(1, 0, 0, 0);
			c[1] = vec128_f32(0, 1, 0, 0);
			c[2] = vec128_f32(0, 0, 1, 0);
			c[3] = vec128_f32(0, 0, 0, 1);
		}

		inline mat128_f32(const float3x3 & rhs) :
			mat128_f32(
				rhs._00, rhs._01, rhs._02, 0,
				rhs._10, rhs._11, rhs._12, 0,
				rhs._20, rhs._21, rhs._22, 0, 
				0, 0, 0, 1) {}

		inline mat128_f32(const float4x4 & rhs) : mat128_f32(rhs.data()) { }

		inline mat128_f32(
			const vec128 & r0,
			const vec128 & r1,
			const vec128 & r2,
			const vec128 & r3)
		{
			c[0] = r0;
			c[1] = r1;
			c[2] = r2;
			c[3] = r3;
		}

		inline mat128_f32(const float4 & r0, const float4 & r1, const float4 & r2, const float4 & r3)
		{
			c[0] = vec128_f32(r0);
			c[1] = vec128_f32(r1);
			c[2] = vec128_f32(r2);
			c[3] = vec128_f32(r3);
		}

		inline mat128_f32(
			float _00, float _01, float _02, float _03,
			float _10, float _11, float _12, float _13,
			float _20, float _21, float _22, float _23,
			float _30, float _31, float _32, float _33)
		{
			c[0] = vec128_f32(_00, _10, _20, _30);
			c[1] = vec128_f32(_01, _11, _21, _31);
			c[2] = vec128_f32(_02, _12, _22, _32);
			c[3] = vec128_f32(_03, _13, _23, _33);
		}

		inline mat128_f32(const float * v)
		{
			c[0] = vec128_f32(v);
			c[1] = vec128_f32(v + 4);
			c[2] = vec128_f32(v + 8);
			c[3] = vec128_f32(v + 12);
		}

		mat128_f32 & FUSE_VECTOR_CALL operator= (mat128 rhs)
		{
			c[0] = rhs.c[0];
			c[1] = rhs.c[1];
			c[2] = rhs.c[2];
			c[3] = rhs.c[3];
			return *this;
		}

		inline mat128_f32 & operator= (const mat128_f32 & rhs)
		{
			c[0] = rhs.c[0];
			c[1] = rhs.c[1];
			c[2] = rhs.c[2];
			c[3] = rhs.c[3];
			return *this;
		}

		inline operator mat128 () const { return reinterpret_cast<const mat128&>(c); }

		union
		{
			vec128   c[4];
			float4x4 m;
			float    f32[16];
		};
	};

}