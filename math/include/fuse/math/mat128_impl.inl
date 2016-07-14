#pragma once

namespace fuse
{

	/* Initializers */

	inline const mat128 & mat128_identity(void)
	{
		static const mat128 t = mat128_f32(1.f);;
		return t;
	}

	inline mat128 FUSE_VECTOR_CALL mat128_load(const float4x4 & lhs)
	{
		mat128 r;
		r.c[0] = vec128_load(transpose(lhs.c[0]));
		r.c[1] = vec128_load(transpose(lhs.c[1]));
		r.c[2] = vec128_load(transpose(lhs.c[2]));
		r.c[3] = vec128_load(transpose(lhs.c[3]));
		return r;
	}

	inline mat128 FUSE_VECTOR_CALL mat128_load(const float3x3 & lhs)
	{
		mat128 r;
		r.c[0] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_load(transpose(lhs.c[0])), vec128_zero());
		r.c[1] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_load(transpose(lhs.c[1])), vec128_zero());
		r.c[2] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_load(transpose(lhs.c[2])), vec128_zero());
		r.c[3] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_zero(), vec128_one());
		return r;
	}

	/* Matrix 4x4 */

	inline mat128 FUSE_VECTOR_CALL mat128_transpose(mat128 lhs)
	{
		mat128 r;

		vec128 t0 = vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_X1, FUSE_Y1>(lhs.c[0], lhs.c[1]);
		vec128 t1 = vec128_shuffle<FUSE_Z0, FUSE_W0, FUSE_Z1, FUSE_W1>(lhs.c[0], lhs.c[1]);

		vec128 t2 = vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_X1, FUSE_Y1>(lhs.c[2], lhs.c[3]);
		vec128 t3 = vec128_shuffle<FUSE_Z0, FUSE_W0, FUSE_Z1, FUSE_W1>(lhs.c[2], lhs.c[3]);

		r.c[0] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t0, t2);
		r.c[1] = vec128_shuffle<FUSE_Y0, FUSE_W0, FUSE_Y1, FUSE_W1>(t0, t2);
		r.c[2] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t1, t3);
		r.c[3] = vec128_shuffle<FUSE_Y0, FUSE_W0, FUSE_Y1, FUSE_W1>(t1, t3);

		return r;
	}

	namespace detail
	{

		/* Intel Inverse of 4x4 Matrix */

#ifdef __MSVC_RUNTIME_CHECKS 
#pragma runtime_checks("u", off)
#endif

		inline void intel_sse_inverse4(const float * src, float * rows, __m128 * pDet)
		{
			__m128 minor0, minor1, minor2, minor3;
			__m128 row0, row1, row2, row3;
			__m128 det, tmp1;

			tmp1   = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src)), (__m64*)(src + 4));
			row1   = _mm_loadh_pi(_mm_loadl_pi(row1, (__m64*)(src + 8)), (__m64*)(src + 12));
			row0   = _mm_shuffle_ps(tmp1, row1, 0x88);
			row1   = _mm_shuffle_ps(row1, tmp1, 0xDD);
			tmp1   = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src + 2)), (__m64*)(src + 6));
			row3   = _mm_loadh_pi(_mm_loadl_pi(row3, (__m64*)(src + 10)), (__m64*)(src + 14));
			row2   = _mm_shuffle_ps(tmp1, row3, 0x88);
			row3   = _mm_shuffle_ps(row3, tmp1, 0xDD);

			tmp1   = _mm_mul_ps(row2, row3);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			minor0 = _mm_mul_ps(row1, tmp1);
			minor1 = _mm_mul_ps(row0, tmp1);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor0 = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
			minor1 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
			minor1 = _mm_shuffle_ps(minor1, minor1, 0x4E);

			tmp1   = _mm_mul_ps(row1, row2);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			minor0 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
			minor3 = _mm_mul_ps(row0, tmp1);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
			minor3 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
			minor3 = _mm_shuffle_ps(minor3, minor3, 0x4E);

			tmp1   = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			row2   = _mm_shuffle_ps(row2, row2, 0x4E);
			minor0 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
			minor2 = _mm_mul_ps(row0, tmp1);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor0 = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
			minor2 = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
			minor2 = _mm_shuffle_ps(minor2, minor2, 0x4E);

			tmp1   = _mm_mul_ps(row0, row1);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);

			minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
			minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
			minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));

			tmp1   = _mm_mul_ps(row0, row3);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
			minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
			minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));

			tmp1   = _mm_mul_ps(row0, row2);
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
			minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
			minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
			tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
			minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
			minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

			det    = _mm_mul_ps(row0, minor0);
			det    = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
			det    = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
			tmp1   = _mm_rcp_ss(det);
			det    = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
			det    = _mm_shuffle_ps(det, det, 0x00);

			minor0 = _mm_mul_ps(det, minor0);
			_mm_storel_pi((__m64*)(rows), minor0);
			_mm_storeh_pi((__m64*)(rows + 2), minor0);

			minor1 = _mm_mul_ps(det, minor1);
			_mm_storel_pi((__m64*)(rows + 4), minor1);
			_mm_storeh_pi((__m64*)(rows + 6), minor1);

			minor2 = _mm_mul_ps(det, minor2);
			_mm_storel_pi((__m64*)(rows + 8), minor2);
			_mm_storeh_pi((__m64*)(rows + 10), minor2);

			minor3 = _mm_mul_ps(det, minor3);
			_mm_storel_pi((__m64*)(rows + 12), minor3);
			_mm_storeh_pi((__m64*)(rows + 14), minor3);

			*pDet = det;
		}

#ifdef  __MSVC_RUNTIME_CHECKS 
#pragma runtime_checks("u", restore)
#endif

	}

	inline mat128 FUSE_VECTOR_CALL mat128_transform4(mat128 lhs, mat128 rhs)
	{
		mat128 r;
		mat128 lhsT = mat128_transpose(lhs);

		{
			vec128 _00 = vec128_dot4(lhsT.c[0], rhs.c[0]);
			vec128 _10 = vec128_dot4(lhsT.c[1], rhs.c[0]);
			vec128 _20 = vec128_dot4(lhsT.c[2], rhs.c[0]);
			vec128 _30 = vec128_dot4(lhsT.c[3], rhs.c[0]);

			vec128 t0 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_00, _10);
			vec128 t1 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_20, _30);

			r.c[0] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t0, t1);
		}

		{
			vec128 _01 = vec128_dot4(lhsT.c[0], rhs.c[1]);
			vec128 _11 = vec128_dot4(lhsT.c[1], rhs.c[1]);
			vec128 _21 = vec128_dot4(lhsT.c[2], rhs.c[1]);
			vec128 _31 = vec128_dot4(lhsT.c[3], rhs.c[1]);

			vec128 t0 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_01, _11);
			vec128 t1 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_21, _31);

			r.c[1] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t0, t1);
		}

		{
			vec128 _02 = vec128_dot4(lhsT.c[0], rhs.c[2]);
			vec128 _12 = vec128_dot4(lhsT.c[1], rhs.c[2]);
			vec128 _22 = vec128_dot4(lhsT.c[2], rhs.c[2]);
			vec128 _32 = vec128_dot4(lhsT.c[3], rhs.c[2]);

			vec128 t0 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_02, _12);
			vec128 t1 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_22, _32);

			r.c[2] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t0, t1);
		}

		{
			vec128 _03 = vec128_dot4(lhsT.c[0], rhs.c[3]);
			vec128 _13 = vec128_dot4(lhsT.c[1], rhs.c[3]);
			vec128 _23 = vec128_dot4(lhsT.c[2], rhs.c[3]);
			vec128 _33 = vec128_dot4(lhsT.c[3], rhs.c[3]);

			vec128 t0 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_03, _13);
			vec128 t1 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_23, _33);

			r.c[3] = vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(t0, t1);
		}

		return r;
	}

	inline mat128 FUSE_VECTOR_CALL mat128_inverse4(mat128 lhs, vec128 * pDeterminant = nullptr)
	{
		mat128 r;
		vec128 det;

		detail::intel_sse_inverse4((float*)&lhs, (float*)&r, &det);

		if (pDeterminant)
		{
			*pDeterminant = det;
		}

		return r;
	}

	inline vec128 FUSE_VECTOR_CALL mat128_determinant3(mat128 lhs)
	{
		__m128 t0 = _mm_shuffle_ps(lhs.c[1], lhs.c[1], _MM_SHUFFLE(3, 0, 2, 1));
		__m128 t1 = _mm_shuffle_ps(lhs.c[2], lhs.c[2], _MM_SHUFFLE(3, 1, 0, 2));

		__m128 t2 = _mm_shuffle_ps(lhs.c[1], lhs.c[1], _MM_SHUFFLE(3, 1, 0, 2));
		__m128 t3 = _mm_shuffle_ps(lhs.c[2], lhs.c[2], _MM_SHUFFLE(3, 0, 2, 1));

		__m128 t4 = _mm_mul_ps(lhs.c[0], t0);
		__m128 t5 = _mm_dp_ps(t4, t1, 0x71);

		__m128 t7 = _mm_mul_ps(lhs.c[0], t2);
		__m128 t8 = _mm_dp_ps(t7, t3, 0x71);

		return vec128_splat<FUSE_X>(_mm_sub_ss(t5, t8));
	}

	inline vec128 FUSE_VECTOR_CALL mat128_determinant4(mat128 lhs)
	{
		/* GLM sse_det_ps */

		__m128 SubE;
		__m128 SubF;
		{

			// First 2 columns
			__m128 Swp2A = _mm_shuffle_ps(lhs.c[2], lhs.c[2], _MM_SHUFFLE(0, 1, 1, 2));
			__m128 Swp3A = _mm_shuffle_ps(lhs.c[3], lhs.c[3], _MM_SHUFFLE(3, 2, 3, 3));

			// Second 2 columns
			__m128 Swp2B = _mm_shuffle_ps(lhs.c[2], lhs.c[2], _MM_SHUFFLE(3, 2, 3, 3));
			__m128 Swp3B = _mm_shuffle_ps(lhs.c[3], lhs.c[3], _MM_SHUFFLE(0, 1, 1, 2));

			// Last 2 rows
			__m128 Swp2C = _mm_shuffle_ps(lhs.c[2], lhs.c[2], _MM_SHUFFLE(0, 0, 1, 2));
			__m128 Swp3C = _mm_shuffle_ps(lhs.c[3], lhs.c[3], _MM_SHUFFLE(1, 2, 0, 0));

			__m128 MulA = _mm_mul_ps(Swp2A, Swp3A);
			__m128 MulB = _mm_mul_ps(Swp2B, Swp3B);
			__m128 MulC = _mm_mul_ps(Swp2C, Swp3C);

			// Columns subtraction
			SubF = _mm_sub_ps(_mm_movehl_ps(MulC, MulC), MulC);
			SubE = _mm_sub_ps(MulA, MulB);

		}

		__m128 DetCof;
		{

			__m128 SubFacA = _mm_shuffle_ps(SubE, SubE, _MM_SHUFFLE(2, 1, 0, 0));
			__m128 SwpFacA = _mm_shuffle_ps(lhs.c[1], lhs.c[1], _MM_SHUFFLE(0, 0, 0, 1));

			__m128 SubTmpB = _mm_shuffle_ps(SubE, SubF, _MM_SHUFFLE(0, 0, 3, 1));
			__m128 SubFacB = _mm_shuffle_ps(SubTmpB, SubTmpB, _MM_SHUFFLE(3, 1, 1, 0));//SubF[0], SubE[3], SubE[3], SubE[1];
			__m128 SwpFacB = _mm_shuffle_ps(lhs.c[1], lhs.c[1], _MM_SHUFFLE(1, 1, 2, 2));

			__m128 SubTmpC = _mm_shuffle_ps(SubE, SubF, _MM_SHUFFLE(1, 0, 2, 2));
			__m128 SubFacC = _mm_shuffle_ps(SubTmpC, SubTmpC, _MM_SHUFFLE(3, 3, 2, 0));
			__m128 SwpFacC = _mm_shuffle_ps(lhs.c[1], lhs.c[1], _MM_SHUFFLE(2, 3, 3, 3));

			__m128 MulFacA = _mm_mul_ps(SwpFacA, SubFacA);
			__m128 MulFacB = _mm_mul_ps(SwpFacB, SubFacB);
			__m128 MulFacC = _mm_mul_ps(SwpFacC, SubFacC);

			__m128 SubRes = _mm_sub_ps(MulFacA, MulFacB);
			__m128 AddRes = _mm_add_ps(SubRes, MulFacC);

			DetCof = _mm_mul_ps(AddRes, _mm_setr_ps(1.0f, -1.0f, 1.0f, -1.0f));


		}

		return _mm_dp_ps(lhs.c[0], DetCof, 0xFF);
	}

	/* Vector operations */

	inline vec128 FUSE_VECTOR_CALL mat128_transform4(vec128 lhs, mat128 rhs)
	{
		vec128 _0 = vec128_dot4(rhs.c[0], lhs);
		vec128 _1 = vec128_dot4(rhs.c[1], lhs);
		vec128 _2 = vec128_dot4(rhs.c[2], lhs);
		vec128 _3 = vec128_dot4(rhs.c[3], lhs);

		vec128 _0011 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_0, _1);
		vec128 _2233 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_2, _3);

		return vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(_0011, _2233);
	}

	inline vec128 FUSE_VECTOR_CALL mat128_transform3(vec128 lhs, mat128 rhs)
	{
		vec128 t = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(lhs, vec128_one());
		return mat128_transform4(t, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL mat128_transform_normal(vec128 lhs, mat128 rhs)
	{
		vec128 _0 = vec128_dot3(rhs.c[0], lhs);
		vec128 _1 = vec128_dot3(rhs.c[1], lhs);
		vec128 _2 = vec128_dot3(rhs.c[2], lhs);

		vec128 _0011 = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_0, _1);
		vec128 _22ww = vec128_shuffle<FUSE_X0, FUSE_X0, FUSE_X1, FUSE_X1>(_2, vec128_one());

		return vec128_shuffle<FUSE_X0, FUSE_Z0, FUSE_X1, FUSE_Z1>(_0011, _22ww);
	}

	/* Operators */

	inline mat128 FUSE_VECTOR_CALL operator* (mat128 lhs, mat128 rhs)
	{
		return mat128_transform4(lhs, rhs);
	}

	inline vec128 FUSE_VECTOR_CALL operator* (vec128 lhs, mat128 rhs)
	{
		return mat128_transform4(lhs, rhs);
	}

}