#pragma once

#include <fuse/math/math.hpp>

#include <fuse/math.hpp>
#include <DirectXCollision.h>

#include <cassert>

namespace fuse
{


	inline float determinant(XMFLOAT3X3 A)
	{
		return A._11 * A._22 * A._33 +
			A._12 * A._23 * A._31 +
			A._13 * A._21 * A._32 -
			A._13 * A._22 * A._31 -
			A._12 * A._21 * A._33 -
			A._11 * A._23 * A._32;
	}

	/* Solves Ax = b in 3 dimensions */

	inline vec128 FUSE_VECTOR_CALL cramer3(mat128 A, vec128 b)
	{
		vec128 det = mat128_det3(A);

		assert(vec128_get_x(det) != 0 && "Singular matrix means the system has infinite solutions.");

		vec128 x = vec128_dp3(b, vec128_cross(A.c[1], A.c[2]));
		vec128 y = vec128_dp3(A.c[0], vec128_cross(b, A.c[2]));
		vec128 z = vec128_dp3(A.c[0], vec128_cross(A.c[1], b));

		return vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_Z1, FUSE_W1>(vec128_permute<FUSE_X0, FUSE_Y1, FUSE_Z0, FUSE_W0>(x, y), z) / det;

		/*XMMATRIX At = XMMatrixTranspose(XMLoadFloat3x3(&A));

		XMVECTOR bVec = to_vector(b);

		float invDetA = 1.f / determinant(A);

		XMVECTOR x = XMVector3Dot(bVec, XMVector3Cross(At.r[1], At.r[2]));
		XMVECTOR y = XMVector3Dot(At.r[0], XMVector3Cross(bVec, At.r[2]));
		XMVECTOR z = XMVector3Dot(At.r[0], XMVector3Cross(At.r[1], bVec));

		return 
			XMVectorScale(XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1Z, XM_PERMUTE_1W>(
				XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(x, y),
				z), invDetA);*/

	}

}