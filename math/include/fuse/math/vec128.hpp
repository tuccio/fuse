#pragma once

#include <xmmintrin.h>

#include "math_fwd.hpp"

#include "matrix.hpp"

#include "vector2.hpp"
#include "vector3.hpp"
#include "vector4.hpp"

#include <cstdint>

#define FUSE_VECTOR_CALL __vectorcall

namespace fuse
{

	struct alignas(16) vec128_f32
	{

		inline vec128_f32(void) = default;
		inline vec128_f32(const vec128_f32 & rhs) : m128(rhs.m128) {}
		inline vec128_f32(vec128_f32 && rhs) : m128(rhs.m128) {}

		inline vec128_f32(float x) : m128(_mm_set1_ps(x)) {}

		inline vec128_f32(vec128 m) : m128(m) {}

		inline vec128_f32(const float2 & v) : vec128_f32(v.x, v.y, 0.f, 1.f) {}
		inline vec128_f32(const float3 & v) : vec128_f32(v.x, v.y, v.z, 1.f) {}
		inline vec128_f32(const float4 & v) : m128(_mm_loadu_ps(v.data())) {}

		inline vec128_f32(float x, float y, float z, float w) : m128(_mm_set_ps(w, z, y, x)) {}
		inline vec128_f32(const float * v) : m128(_mm_loadu_ps(v)) {}

		inline operator vec128 () const { return m128; }

		vec128_f32 & FUSE_VECTOR_CALL operator= (vec128 m) { m128 = m; return *this; }

		union
		{
			vec128 m128;

			float2 v2f;
			float3 v3f;
			float4 v4f;

			float  f32[4];
		};

	};

}