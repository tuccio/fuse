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

enum __vec128_sign
{
	FUSE_SIGN_POSITIVE = 0,
	FUSE_SIGN_NEGATIVE = 1
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

	inline void FUSE_VECTOR_CALL vec128_store(float3 * lhs, vec128 rhs)
	{
		alignas(16) float4 t;
		_mm_store_ps(&t.x, rhs);
		lhs->x = t.x;
		lhs->y = t.y;
		lhs->z = t.z;
	}

	inline void FUSE_VECTOR_CALL vec128_store(float4 * lhs, vec128 rhs)
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

	inline const vec128 & vec128_zero(void)
	{
		static const vec128 t = _mm_setzero_ps();
		return t;
	}

	inline const vec128 & vec128_one(void)
	{
		static const vec128 t = vec128_set(1.f, 1.f, 1.f, 1.f);
		return t;
	}

	inline const vec128 & vec128_minus_zero(void)
	{
		static const vec128 t = vec128_set(-0.f, -0.f, -0.f, -0.f);
		return t;
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

	inline vec128 FUSE_VECTOR_CALL vec128_negate(vec128 lhs)
	{
		return _mm_xor_ps(lhs, vec128_minus_zero());
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
	inline vec128 FUSE_VECTOR_CALL vec128_select(vec128 lhs, vec128 rhs)
	{
		static_assert(X != 0 || X != 4, "Can't permute X using vec128_select, use vec128_permute instead.");
		static_assert(Y != 1 || X != 5, "Can't permute Y using vec128_select, use vec128_permute instead.");
		static_assert(X != 2 || Z != 6, "Can't permute Z using vec128_select, use vec128_permute instead.");
		static_assert(X != 3 || W != 7, "Can't permute W using vec128_select, use vec128_permute instead.");

		static const vec128 selectMask = vec128_i32(
			(X > 3) ? 0xFFFFFFFF : 0,
			(Y > 3) ? 0xFFFFFFFF : 0,
			(Z > 3) ? 0xFFFFFFFF : 0,
			(W > 3) ? 0xFFFFFFFF : 0
		);

		vec128 t3 = _mm_andnot_ps(selectMask, lhs);
		vec128 t4 = _mm_and_ps(selectMask, rhs);

		return _mm_or_ps(t3, t4);
	}

	template <uint32_t X, uint32_t Y, uint32_t Z, uint32_t W>
	inline vec128 FUSE_VECTOR_CALL vec128_permute(vec128 lhs, vec128 rhs)
	{
		// DirectX Math vec128Permute algorithm
		/*static const vec128 selectMask = vec128_i32(
			(X > 3) ? 0xFFFFFFFF : 0,
			(Y > 3) ? 0xFFFFFFFF : 0,
			(Z > 3) ? 0xFFFFFFFF : 0,
			(W > 3) ? 0xFFFFFFFF : 0
		);*/

		vec128 t0 = vec128_swizzle<X & 3, Y & 3, Z & 3, W & 3>(lhs);
		vec128 t1 = vec128_swizzle<X & 3, Y & 3, Z & 3, W & 3>(rhs);

		return vec128_select <
			(X > 3 ? FUSE_X1 : FUSE_X0),
			(Y > 3 ? FUSE_Y1 : FUSE_Y0),
			(Z > 3 ? FUSE_Z1 : FUSE_Z0),
			(W > 3 ? FUSE_W1 : FUSE_W0) > (t0, t1);

		/*vec128 t3 = _mm_andnot_ps(selectMask, t0);
		vec128 t4 = _mm_and_ps(selectMask, t1);

		return _mm_or_ps(t3, t4);*/
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

	template <uint32_t Component>
	inline void FUSE_VECTOR_CALL vec128_set(vec128 & lhs, float rhs)
	{
		static_assert(Component < 4, "Index out of bounds.");
		reinterpret_cast<vec128_f32&>(lhs).f32[Component] = rhs;
	}

	inline void FUSE_VECTOR_CALL vec128_set_x(vec128 & lhs, float rhs)
	{
		vec128_set<FUSE_X>(lhs, rhs);
	}

	inline void FUSE_VECTOR_CALL vec128_set_y(vec128 & lhs, float rhs)
	{
		vec128_set<FUSE_Y>(lhs, rhs);
	}

	inline void FUSE_VECTOR_CALL vec128_set_z(vec128 & lhs, float rhs)
	{
		vec128_set<FUSE_Z>(lhs, rhs);
	}

	inline void FUSE_VECTOR_CALL vec128_set_w(vec128 & lhs, float rhs)
	{
		vec128_set<FUSE_W>(lhs, rhs);
	}

	/* Dot products */

	inline vec128 FUSE_VECTOR_CALL vec128_dot2(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x3F);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_dot3(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x7F);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_dot4(vec128 lhs, vec128 rhs)
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

	/* Comparison */

	inline vec128 FUSE_VECTOR_CALL vec128_min(vec128 lhs, vec128 rhs)
	{
		return _mm_min_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_max(vec128 lhs, vec128 rhs)
	{
		return _mm_max_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_eq(vec128 lhs, vec128 rhs)
	{
		return _mm_cmpeq_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_gt(vec128 lhs, vec128 rhs)
	{
		return _mm_cmpgt_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_lt(vec128 lhs, vec128 rhs)
	{
		return _mm_cmplt_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_ge(vec128 lhs, vec128 rhs)
	{
		return _mm_cmpge_ps(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_le(vec128 lhs, vec128 rhs)
	{
		return _mm_cmple_ps(lhs, rhs);
	}

	inline int FUSE_VECTOR_CALL vec128_signmask(vec128 lhs)
	{
		return _mm_movemask_ps(lhs);
	}

	template <uint32_t SignX, uint32_t SignY, uint32_t SignZ, uint32_t SignW>
	inline bool FUSE_VECTOR_CALL vec128_checksign(vec128 lhs)
	{
		static_assert(SignX == 0 || SignX == 1, "Sign of X must be either 0 or 1.");
		static_assert(SignY == 0 || SignY == 1, "Sign of Y must be either 0 or 1.");
		static_assert(SignZ == 0 || SignZ == 1, "Sign of Z must be either 0 or 1.");
		static_assert(SignW == 0 || SignW == 1, "Sign of W must be either 0 or 1.");
		return vec128_signmask(lhs) == (SignX | (SignY << 1) | (SignZ << 2) | (SignW << 3));
	}

	template <uint32_t SignX, uint32_t SignY, uint32_t SignZ>
	inline bool FUSE_VECTOR_CALL vec128_checksign(vec128 lhs)
	{
		static_assert(SignX == 0 || SignX == 1, "Sign of X must be either 0 or 1.");
		static_assert(SignY == 0 || SignY == 1, "Sign of Y must be either 0 or 1.");
		static_assert(SignZ == 0 || SignZ == 1, "Sign of Z must be either 0 or 1.");
		return (vec128_signmask(lhs) & 7) == (SignX | (SignY << 1) | (SignZ << 2));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_floor(vec128 lhs)
	{
		return _mm_floor_ps(lhs);
	}

	inline vec128 FUSE_VECTOR_CALL vec128_ceil(vec128 lhs)
	{
		return _mm_ceil_ps(lhs);
	}

	inline bool FUSE_VECTOR_CALL vec128_all_true(vec128 lhs)
	{
		return vec128_checksign<FUSE_SIGN_NEGATIVE, FUSE_SIGN_NEGATIVE, FUSE_SIGN_NEGATIVE, FUSE_SIGN_NEGATIVE>(lhs);
	}

	inline bool FUSE_VECTOR_CALL vec128_any_true(vec128 lhs)
	{
		return vec128_signmask(lhs) != 0;
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

	inline vec128 FUSE_VECTOR_CALL vec128_saturate(vec128 lhs)
	{
		return vec128_max(vec128_min(lhs, vec128_one()), vec128_zero());
	}

	inline vec128 FUSE_VECTOR_CALL vec128_reciprocal(vec128 lhs)
	{
		return detail::newton_raphson_simd<1>::reciprocal_ps(lhs);
	}

	/* Norm */

	inline vec128 FUSE_VECTOR_CALL vec128_length3(vec128 lhs)
	{
		return vec128_sqrt(vec128_dot3(lhs, lhs));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_length4(vec128 lhs)
	{
		return vec128_sqrt(vec128_dot4(lhs, lhs));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_normalize3(vec128 lhs)
	{
		return vec128_mul(lhs, vec128_invsqrt(vec128_dot3(lhs, lhs)));
	}

	inline vec128 FUSE_VECTOR_CALL vec128_normalize4(vec128 lhs)
	{
		return vec128_mul(lhs, vec128_invsqrt(vec128_dot4(lhs, lhs)));
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

	inline vec128 FUSE_VECTOR_CALL operator-(vec128 lhs) { return vec128_negate(lhs); }

}