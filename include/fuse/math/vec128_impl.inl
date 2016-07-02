enum vec128_components
{
	FUSE_X, FUSE_Y, FUSE_Z, FUSE_W
};

enum vec128_shuffle0
{
	FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W0
};

enum vec128_shuffle1
{
	FUSE_X1, FUSE_Y1, FUSE_Z1, FUSE_W1
};

namespace fuse
{

	/* Stream */

	std::ostream & operator<< (std::ostream & os, const vec128_f32 & v)
	{
		return os << v.v4f;
	}

	/* Initializers */

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

	vec128 FUSE_VECTOR_CALL vec128_add(vec128 lhs, vec128 rhs)
	{
		return _mm_add_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_sub(vec128 lhs, vec128 rhs)
	{
		return _mm_sub_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_mul(vec128 lhs, vec128 rhs)
	{
		return _mm_mul_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_div(vec128 lhs, vec128 rhs)
	{
		return _mm_div_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_neg(vec128 lhs)
	{
		return _mm_xor_ps(lhs, _mm_set1_ps(-0.f));
	}

	/* Logic */

	vec128 FUSE_VECTOR_CALL vec128_and(vec128 lhs, vec128 rhs)
	{
		return _mm_and_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_or(vec128 lhs, vec128 rhs)
	{
		return _mm_or_ps(lhs, rhs);
	}

	vec128 FUSE_VECTOR_CALL vec128_xor(vec128 lhs, vec128 rhs)
	{
		return _mm_xor_ps(lhs, rhs);
	}

	/* Shuffling */

	template <vec128_shuffle0 X, vec128_shuffle0 Y, vec128_shuffle1 Z, vec128_shuffle1 W>
	vec128 FUSE_VECTOR_CALL vec128_shuffle(vec128 lhs, vec128 rhs)
	{
		return _mm_shuffle_ps(lhs, rhs, _MM_SHUFFLE(W, Z, Y, X));
	}

	template <vec128_components X, vec128_components Y, vec128_components Z, vec128_components W>
	vec128 FUSE_VECTOR_CALL vec128_swizzle(vec128 lhs)
	{
		return _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(W, Z, Y, X));
	}

	template <vec128_components XYZW>
	vec128 FUSE_VECTOR_CALL vec128_splat(vec128 lhs)
	{
		return vec128_swizzle<XYZW, XYZW, XYZW, XYZW>(lhs);
	}

	/* Access */

	template <vec128_components Component>
	float FUSE_VECTOR_CALL vec128_get(vec128 lhs)
	{
		return reinterpret_cast<const vec128_f32&>(lhs).f32[Component];
	}

	float FUSE_VECTOR_CALL vec128_get_x(vec128 lhs)
	{
		return vec128_get<FUSE_X>(lhs);
	}

	float FUSE_VECTOR_CALL vec128_get_y(vec128 lhs)
	{
		return vec128_get<FUSE_Y>(lhs);
	}

	float FUSE_VECTOR_CALL vec128_get_z(vec128 lhs)
	{
		return vec128_get<FUSE_Z>(lhs);
	}

	float FUSE_VECTOR_CALL vec128_get_w(vec128 lhs)
	{
		return vec128_get<FUSE_W>(lhs);
	}

	/* Dot products */

	vec128 FUSE_VECTOR_CALL vec128_dp2(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x3F);
	}

	vec128 FUSE_VECTOR_CALL vec128_dp3(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0x7F);
	}

	vec128 FUSE_VECTOR_CALL vec128_dp4(vec128 lhs, vec128 rhs)
	{
		return _mm_dp_ps(lhs, rhs, 0xFF);
	}

	/* Cross product */

	vec128 FUSE_VECTOR_CALL vec128_cross(vec128 lhs, vec128 rhs)
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

	/* Operators */

	vec128 FUSE_VECTOR_CALL operator+(vec128 lhs, vec128 rhs) { return vec128_add(lhs, rhs); }
	vec128 FUSE_VECTOR_CALL operator-(vec128 lhs, vec128 rhs) { return vec128_sub(lhs, rhs); }
	vec128 FUSE_VECTOR_CALL operator*(vec128 lhs, vec128 rhs) { return vec128_mul(lhs, rhs); }
	vec128 FUSE_VECTOR_CALL operator/(vec128 lhs, vec128 rhs) { return vec128_div(lhs, rhs); }

	vec128 FUSE_VECTOR_CALL operator-(vec128 lhs) { return vec128_neg(lhs); }

}