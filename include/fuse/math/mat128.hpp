#pragma once

#include "vec128.hpp"

#include "matrix4x4.hpp"

namespace fuse
{

	struct alignas(16) mat128
	{
		vec128 m128[4];
	};

	struct alignas(16) mat128_f32
	{

		inline mat128_f32(void) = default;

		inline mat128_f32(const mat128 & rhs)
		{
			m128[0] = rhs.m128[0];
			m128[1] = rhs.m128[1];
			m128[2] = rhs.m128[2];
			m128[3] = rhs.m128[3];
		}

		inline mat128_f32(const mat128_f32 & rhs)
		{
			m128[0] = rhs.m128[0];
			m128[1] = rhs.m128[1];
			m128[2] = rhs.m128[2];
			m128[3] = rhs.m128[3];
		}

		inline mat128_f32(mat128_f32 && rhs)
		{
			m128[0] = rhs.m128[0];
			m128[1] = rhs.m128[1];
			m128[2] = rhs.m128[2];
			m128[3] = rhs.m128[3];
		}

		inline mat128_f32(float d)
		{
			m128[0] = vec128_f32(1, 0, 0, 0);
			m128[1] = vec128_f32(0, 1, 0, 0);
			m128[2] = vec128_f32(0, 0, 1, 0);
			m128[3] = vec128_f32(0, 0, 0, 1);
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
			m128[0] = r0;
			m128[1] = r1;
			m128[2] = r2;
			m128[3] = r3;
		}

		inline mat128_f32(const float4 & r0, const float4 & r1, const float4 & r2, const float4 & r3)
		{
			m128[0] = vec128_f32(r0);
			m128[1] = vec128_f32(r1);
			m128[2] = vec128_f32(r2);
			m128[3] = vec128_f32(r3);
		}

		inline mat128_f32(
			float _00, float _01, float _02, float _03,
			float _10, float _11, float _12, float _13,
			float _20, float _21, float _22, float _23,
			float _30, float _31, float _32, float _33)
		{
			m128[0] = vec128_f32(_00, _10, _20, _30);
			m128[1] = vec128_f32(_01, _11, _21, _31);
			m128[2] = vec128_f32(_02, _12, _22, _32);
			m128[3] = vec128_f32(_03, _13, _23, _33);
		}

		inline mat128_f32(const float * v)
		{
			m128[0] = vec128_f32(v);
			m128[1] = vec128_f32(v + 4);
			m128[2] = vec128_f32(v + 8);
			m128[3] = vec128_f32(v + 12);
		}

		mat128_f32 & FUSE_VECTOR_CALL operator= (mat128 rhs)
		{
			m128[0] = rhs.m128[0];
			m128[1] = rhs.m128[1];
			m128[2] = rhs.m128[2];
			m128[3] = rhs.m128[3];
			return *this;
		}

		inline mat128_f32 & operator= (const mat128_f32 & rhs)
		{
			m128[0] = rhs.m128[0];
			m128[1] = rhs.m128[1];
			m128[2] = rhs.m128[2];
			m128[3] = rhs.m128[3];
			return *this;
		}

		inline operator mat128 () const { return reinterpret_cast<const mat128&>(m128); }

		union
		{
			vec128   m128[4];
			float4x4 m;
			float    f32[16];
		};
	};

}

#include "mat128_impl.inl"