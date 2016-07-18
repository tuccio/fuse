#pragma once

#include <fuse/math.hpp>

namespace fuse
{
	
	void decompose_affine(const float4x4 & m, float3 * pScale, quaternion * pRotation, float3 * pTranslation)
	{
		float3 scale, translation;
		quaternion rotation;

		// Extract translation

		translation = float3(m._30, m._31, m._32);

		// Extract scale

		float3x3 SR = transpose(to_float3x3(m));

		scale.x = length(SR.c[0]);
		scale.y = length(SR.c[1]);
		scale.z = length(SR.c[2]);

		float det = determinant(SR);

		if (det < .0f)
		{
			scale.x = -scale.x;
		}

		// Extract rotation

		float3x3 R;

		R.c[0] = SR.c[0] / scale.x;
		R.c[1] = SR.c[1] / scale.y;
		R.c[2] = SR.c[2] / scale.z;

		rotation = to_quaternion(transpose(R));

		// Output

		*pScale       = scale;
		*pRotation    = rotation;
		*pTranslation = translation;
	}

	void FUSE_VECTOR_CALL decompose_affine(mat128 m, vec128 * pScale, vec128 * pRotation, vec128 * pTranslation)
	{
		vec128 scale, rotation, translation;

		// Extract translation

		mat128 mt = mat128_transpose(m);
		translation = mt.c[3];

		// Extract scale

		vec128 t0 = vec128_length3(mt.c[0]);
		vec128 t1 = vec128_length3(mt.c[1]);
		vec128 t2 = vec128_length3(mt.c[2]);

		scale = vec128_permute<FUSE_X0, FUSE_Y1, FUSE_Z0, FUSE_W1>(vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_Z1, FUSE_W1>(t0, t2), t1);

		vec128 det  = mat128_determinant3(mt);
		vec128 mask = vec128_and(det, vec128_set(-0.f, 0.f, 0.f, 0.f));
		scale = vec128_xor(scale, mask);

		// Extract rotation

		mat128 R;

		vec128 rcpScale = vec128_reciprocal(scale);

		R.c[0] = mt.c[0] * vec128_splat<FUSE_X>(rcpScale);
		R.c[1] = mt.c[1] * vec128_splat<FUSE_Y>(rcpScale);
		R.c[2] = mt.c[2] * vec128_splat<FUSE_Z>(rcpScale);

		rotation = to_quat128(R);

		// Output

		*pScale       = scale;
		*pRotation    = rotation;
		*pTranslation = translation;
	}
	
}