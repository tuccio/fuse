#include "newton_raphson.hpp"

namespace fuse
{

	/* Initializers */

	inline vec128 quat128_set(float w, float x, float y, float z)
	{
		return vec128_f32(x, y, z, w);
	}

	inline vec128 quat128_load(const quaternion & q)
	{
		return _mm_loadu_ps(q.m);
	}

	inline vec128 quat128_identity(void)
	{
		return quat128_set(1, 0, 0, 0);
	}

	/* Arithmetic */

	inline vec128 FUSE_VECTOR_CALL quat128_mul(vec128 lhs, vec128 rhs)
	{
		vec128 m0, m1, m2, m3;

		{
			vec128 t0 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3, 3, 3, 3));
			vec128 t1 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(2, 3, 0, 1));
			m0 = _mm_mul_ps(t0, t1);
		}
		
		{
			vec128 t3 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(0, 0, 0, 0));
			vec128 t4 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(1, 0, 3, 2));
			m1 = _mm_mul_ps(t3, t4);
		}

		{
			vec128 t5 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(1, 1, 1, 1));
			vec128 t6 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(2, 0, 3, 1));
			m2 = _mm_mul_ps(t5, t6);
		}		

		{
			vec128 t7 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(2, 2, 2, 2));
			vec128 t8 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3, 2, 0, 1));
			m3 = _mm_mul_ps(t7, t8);
		}
		
		vec128 r = _mm_addsub_ps(m0, m1);

		r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 3, 0, 2));
		r = _mm_addsub_ps(r, m2);

		r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 0, 1, 3));
		r = _mm_addsub_ps(r, m3);

		r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 3, 1, 0));

		return r;
	}

	inline vec128 FUSE_VECTOR_CALL quat128_inverse(vec128 lhs)
	{
		vec128 t0 = vec128_dot4(lhs, lhs);
		vec128 t1 = detail::newton_raphson_simd<1>::reciprocal_ps(t0);
		return _mm_mul_ps(t0, t1);
	}

	inline vec128 FUSE_VECTOR_CALL quat128_conjugate(vec128 lhs)
	{
		static const vec128 signMask = _mm_set_ps(-0.f, -0.f, -0.f, 0.f);
		return _mm_xor_ps(signMask, lhs);
	}

	inline vec128 FUSE_VECTOR_CALL quat128_transform(vec128 lhs, vec128 rhs)
	{
		/*vec128 t0 = _mm_castsi128_ps(_mm_set_epi32(0x0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF));
		vec128 pq = _mm_and_ps(x, t0);
		return quat128_mul(quat128_mul(q, pq), quat128_conjugate(q));*/

		vec128 uv = 2.f * vec128_cross(rhs, lhs);
		return lhs + vec128_splat<FUSE_W>(rhs) * uv + vec128_cross(rhs, uv);
	}

	inline vec128 FUSE_VECTOR_CALL quat128_norm(vec128 lhs)
	{
		return vec128_sqrt(vec128_dot4(lhs, lhs));
	}

	inline vec128 FUSE_VECTOR_CALL quat128_normalize(vec128 lhs)
	{
		return lhs * vec128_invsqrt(vec128_dot4(lhs, lhs));
	}

	/* Conversions */

	inline quaternion FUSE_VECTOR_CALL to_quaternion(vec128 lhs)
	{
		return reinterpret_cast<const quaternion&>(lhs);
	}

	inline vec128 to_quat128(const quaternion & lhs)
	{
		return _mm_loadu_ps(&lhs.x);
	}

	inline vec128 FUSE_VECTOR_CALL to_quat128(mat128 lhs)
	{
		return to_quat128(to_quaternion(reinterpret_cast<const float4x4&>(lhs)));
	}
}