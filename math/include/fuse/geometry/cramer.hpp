#pragma once

#include <fuse/math.hpp>

#include <cassert>

namespace fuse
{

	/* Solves Ax = b in 3 dimensions */

	inline vec128 FUSE_VECTOR_CALL cramer3(mat128 A, vec128 b)
	{
		vec128 det = mat128_determinant3(A);

		assert(vec128_get_x(det) != 0 && "Singular matrix means the system has infinite solutions.");

		vec128 x = vec128_dot3(b, vec128_cross(A.c[1], A.c[2]));
		vec128 y = vec128_dot3(A.c[0], vec128_cross(b, A.c[2]));
		vec128 z = vec128_dot3(A.c[0], vec128_cross(A.c[1], b));

		return vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_Z1, FUSE_W1>(vec128_permute<FUSE_X0, FUSE_Y1, FUSE_Z0, FUSE_W0>(x, y), z) / det;

		/*XMMATRIX At = XMMatrixTranspose(XMLoadFloat3x3(&A));

		vec128 bVec = to_vector(b);

		float invDetA = 1.f / determinant(A);

		vec128 x = vec1283Dot(bVec, vec1283Cross(At.r[1], At.r[2]));
		vec128 y = vec1283Dot(At.r[0], vec1283Cross(bVec, At.r[2]));
		vec128 z = vec1283Dot(At.r[0], vec1283Cross(At.r[1], bVec));

		return 
			vec128Scale(vec128Permute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1Z, XM_PERMUTE_1W>(
				vec128Permute<XM_PERMUTE_0X, XM_PERMUTE_1Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(x, y),
				z), invDetA);*/

	}

}