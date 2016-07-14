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
	
}