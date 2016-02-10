#pragma once

#include <fuse/math.hpp>
#include <DirectXCollision.h>


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

	inline XMVECTOR cramer(XMFLOAT3X3 A, XMFLOAT3 b)
	{

		XMMATRIX At = XMMatrixTranspose(XMLoadFloat3x3(&A));

		XMVECTOR bVec = to_vector(b);

		float invDetA = 1.f / determinant(A);

		XMVECTOR x = XMVector3Dot(bVec, XMVector3Cross(At.r[1], At.r[2]));
		XMVECTOR y = XMVector3Dot(At.r[0], XMVector3Cross(bVec, At.r[2]));
		XMVECTOR z = XMVector3Dot(At.r[0], XMVector3Cross(At.r[1], bVec));

		return 
			XMVectorScale(XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1Z, XM_PERMUTE_1W>(
				XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(x, y),
				z), invDetA);

	}

}