#pragma once

#include <fuse/math/newton_raphson.hpp>

enum __vec128_components
{
	FUSE_X, FUSE_Y, FUSE_Z, FUSE_W
};

enum __vec128_permutation
{
	FUSE_X0 = 0, FUSE_Y0 = 1, FUSE_Z0 = 2, FUSE_W0 = 3,
	FUSE_X1 = 4, FUSE_Y1 = 5, FUSE_Z1 = 6, FUSE_W1 = 7
};

namespace fuse
{

	/* Initializers */

	inline vec128 vec128_set(float x, float y, float z, float w)
	{
		return _mm_set_ps(w, z, y, x);
	}

	inline vec128 vec128_load(const float3 & lhs)
	{
		return _mm_set_ps(1.f, lhs.z, lhs.y, lhs.x);
	}

	inline vec128 vec128_load(const float4 & lhs)
	{
		return _mm_loadu_ps(&lhs.x);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_store(float3 * lhs, vec128 rhs)
	{
		alignas(16) float4 t;
		_mm_store_ps(&t.x, rhs);
		lhs->x = t.x;
		lhs->y = t.y;
		lhs->z = t.z;
	}

	inline vec128 FUSE_VECTOR_CALL vec128_store(float4 * lhs, vec128 rhs)
	{
		_mm_storeu_ps(&lhs->x, rhs);
	}

	inline vec128 vec128_i32(int x)
	{
		return _mm_castsi128_ps(_mm_set1_epi32(x));
	}

	inline vec128 vec128_i32(int x, int y, int z, int w)
	{
		return _mm_castsi128_ps(_mm_set_epi32(w, z, y, x));
	}

	inline vec128 vec128_zero(void)
	{
		return _mm_setzero_ps();
	}

	/* Arithmetic */

	inline vec128 FUSE_VECTOR_CALL vec128_add(vec128 lhs, vec128 rhs)
	{
		return _mm_add_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_sub(vec128 lhs, vec128 rhs)
	{
		return _mm_sub_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_mul(vec128 lhs, vec128 rhs)
	{
		return _mm_mul_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_div(vec128 lhs, vec128 rhs)
	{
		return _mm_div_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_neg(vec128 lhs)
	{
		return _mm_xor_ps(lhs, _mm_set1_ps(-0.f));
	}

	/* Logic */

	inline vec128 FUSE_VECTOR_CALL vec128_and(vec128 lhs, vec128 rhs)
	{
		return _mm_and_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_or(vec128 lhs, vec128 rhs)
	{
		return _mm_or_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_xor(vec128 lhs, vec128 rhs)
	{
		return _mm_xor_ps(lhs, rhs);
	}

	/* Shuffling */

	template <uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
	inline vec128 FUSE_VECTOR_CALL vec128_shuffle(vec128 lhs, vec128 rhs)
	{
		static_assert(X < 4, "X out of bounds.");
		static_assert(Y < 4, "Y out of bounds.");
		static_assert(Z > 3 && Z < 8, "Z out of bounds.");
		static_assert(W > 3 && Z < 8, "W out of bounds.");
		return _mm_shuffle_ps(lhs, rhs, _MM_SHUFFLE(W & 3, Z & 3, Y, X));
	}

	template <uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
	inline vec128 FUSE_VECTOR_CALL vec128_swizzle(vec128 lhs)
	{
		static_assert(X < 4, "X out of bounds.");
		static_assert(Y < 4, "Y out of bounds.");
		static_assert(Z < 4, "Z out of bounds.");
		static_assert(W < 4, "W out of bounds.");
		return _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(W, Z, Y, X));
	}

	template <uint32_t XYZW>
	inline vec128 FUSE_VECTOR_CALL vec128_splat(vec128 lhs)
	{
		static_assert(XYZW < 4, "Index out of bounds.");
		return vec128_swizzle<XYZW, XYZW, XYZW, XYZW>(lhs);
	}

	template <uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
	inline vec128 FUSE_VECTOR_CALL vec128_permute(vec128 lhs, vec128 rhs)
	{
		// DirectX Math XMVectorPermute algorithm
		static const vec128 selectMask = vec128_i32(
			(X > 3) ? 0xFFFFFFFF : 0,
			(Y > 3) ? 0xFFFFFFFF : 0,
			(Z > 3) ? 0xFFFFFFFF : 0,
			(W > 3) ? 0xFFFFFFFF : 0
		);

		vec128 t0 = vec128_swizzle<X & 3, Y & 3, Z & 3, W & 3>(lhs);
		vec128 t1 = vec128_swizzle<X & 3, Y & 3, Z & 3, W & 3>(rhs);

		vec128 t3 = _mm_andnot_ps(selectMask, t0);
		vec128 t4 = _mm_and_ps(selectMask, t1);

		return _mm_or_ps(t3, t4);
	}


	/* Access */

	template <uint32_t Component>
	inline float FUSE_VECTOR_CALL vec128_get(vec128 lhs)
	{
		static_assert(Component < 4, "Index out of bounds.");
		return reinterpret_cast<const vec128_f32&>(lhs).f32[Component];
	}

	inline float FUSE_VECTOR_CALL vec128_get_x(vec128 lhs)
	{
		return vec128_get<FUSE_X>(lhs);
	}

	inline float FUSE_VECTOR_CALL vec128_get_y(vec128 lhs)
	{
		return vec128_get<FUSE_Y>(lhs);
	}

	inline float FUSE_VECTOR_CALL vec128_get_z(vec128 lhs)
	{
		return vec128_get<FUSE_Z>(lhs);
	}

	inline float FUSE_VECTOR_CALL vec128_get_w(vec128 lhs)
	{
		return vec128_get<FUSE_W>(lhs);
	}

	/* Functions */

	inline vec128 FUSE_VECTOR_CALL vec128_invsqrt(vec128 lhs)
	{
		return detail::newton_raphson_simd<1>::reciprocal_square_root_ps(lhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_sqrt(vec128 lhs)
	{
		return detail::newton_raphson_simd<1>::square_root_ps(lhs);
	}

	/* Dot products */

	inline vec128 FUSE_VECTOR_CALL vec128_dp2(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x3F);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_dp3(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x7F);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_dp4(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0xFF);
	}

	/* Cross product */

	inline vec128 FUSE_VECTOR_CALL vec128_cross(vec128 lhs, vec128 rhs)
	{
		// t0 = (y0, z0, x0)
		vec128 t0 = vec128_swizzle<FUSE_Y, FUSE_Z, FUSE_X, FUSE_X>(lhs);

		// t1 = (z1, x1, y1)
		vec128 t1 = vec128_swizzle<FUSE_Z, FUSE_X, FUSE_Y, FUSE_Y>(rhs);

		// t2 = (z0, x0, y1)
		vec128 t2 = vec128_swizzle<FUSE_Z, FUSE_X, FUSE_Y, FUSE_Y>(lhs);

		// t3 = (y1, z1, x1)
		vec128 t3 = vec128_swizzle<FUSE_Y, FUSE_Z, FUSE_X, FUSE_X>(rhs);

		return vec128_sub(vec128_mul(t0, t1), vec128_mul(t2, t3));
	}

	/* Norm */

	inline vec128 FUSE_VECTOR_CALL vec128_len3(vec128 lhs)
	{
		return vec128_sqrt(vec128_dp3(lhs, lhs));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_len4(vec128 lhs)
	{
		return vec128_sqrt(vec128_dp4(lhs, lhs));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_normalize3(vec128 lhs)
	{
		return vec128_mul(lhs, vec128_invsqrt(vec128_dp3(lhs, lhs)));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_normalize4(vec128 lhs)
	{
		return vec128_mul(lhs, vec128_invsqrt(vec128_dp4(lhs, lhs)));
	}

	/* Comparison */

	inline vec128 FUSE_VECTOR_CALL vec128_min(vec128 lhs, vec128 rhs)
	{
		return _mm_min_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_max(vec128 lhs, vec128 rhs)
	{
		return _mm_max_ps(lhs, rhs);
	}

	/* Operators */

	inline vec128 FUSE_VECTOR_CALL operator+(vec128 lhs, vec128 rhs) { return vec128_add(lhs, rhs); }
	inline vec128 FUSE_VECTOR_CALL operator-(vec128 lhs, vec128 rhs) { return vec128_sub(lhs, rhs); }
	inline vec128 FUSE_VECTOR_CALL operator*(vec128 lhs, vec128 rhs) { return vec128_mul(lhs, rhs); }
	inline vec128 FUSE_VECTOR_CALL operator/(vec128 lhs, vec128 rhs) { return vec128_div(lhs, rhs); }

	inline vec128 FUSE_VECTOR_CALL operator*(vec128 lhs, float rhs) { return vec128_mul(lhs, vec128_set(rhs, rhs, rhs, rhs)); }
	inline vec128 FUSE_VECTOR_CALL operator*(float lhs, vec128 rhs) { return vec128_mul(vec128_set(lhs, lhs, lhs, lhs), rhs); }

	inline vec128 FUSE_VECTOR_CALL operator/(vec128 lhs, float rhs) { return vec128_div(lhs, vec128_set(rhs, rhs, rhs, rhs)); }
	inline vec128 FUSE_VECTOR_CALL operator/(float lhs, vec128 rhs) { return vec128_div(vec128_set(lhs, lhs, lhs, lhs), rhs); }

	inline vec128 FUSE_VECTOR_CALL operator-(vec128 lhs) { return vec128_neg(lhs); }

	/* Conversions */

	/*inline vec128 FUSE_VECTOR_CALL to_vec128(vec128 lhs)
	{
		return lhs;
	}

	inline vec128 to_vec128(const float3 & lhs)
	{
		return vec128_load(lhs);
	}

	inline vec128 to_vec128(const float4 & lhs)
	{
		return vec128_load(lhs);
	}
*/
}