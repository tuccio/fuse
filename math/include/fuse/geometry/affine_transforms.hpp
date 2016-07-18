#pragma once

namespace fuse
{
	
	inline mat128 FUSE_VECTOR_CALL to_translation4(vec128 lhs)
	{
		mat128 m = mat128_identity();
		m.c[3] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(lhs, vec128_one());
		return mat128_transpose(m);
	}

	inline mat128 FUSE_VECTOR_CALL to_scale4(vec128 lhs)
	{
		mat128 m;
		vec128 zero = vec128_zero();
		m.c[0] = vec128_select<FUSE_X0, FUSE_Y1, FUSE_Z1, FUSE_W1>(lhs, zero);
		m.c[1] = vec128_select<FUSE_X1, FUSE_Y0, FUSE_Z1, FUSE_W1>(lhs, zero);
		m.c[2] = vec128_select<FUSE_X1, FUSE_Y1, FUSE_Z0, FUSE_W1>(lhs, zero);
		m.c[3] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(zero, vec128_one());
		return m;
	}

	inline float3x3 to_rotation3(const quaternion & lhs)
	{
		const float tx = 2.f * lhs.x;
		const float ty = 2.f * lhs.y;
		const float tz = 2.f * lhs.z;
		const float twx = tx * lhs.w;
		const float twy = ty * lhs.w;
		const float twz = tz * lhs.w;
		const float txx = tx * lhs.x;
		const float txy = ty * lhs.x;
		const float txz = tz * lhs.x;
		const float tyy = ty * lhs.y;
		const float tyz = tz * lhs.y;
		const float tzz = tz * lhs.z;

		return float3x3(
			1.f - (tyy + tzz), txy + twz, txz - twy,
			txy - twz, 1.f - (txx + tzz), tyz + twx,
			txz + twy, tyz - twx, 1.f - (txx + tyy)
		);
	}

	inline float4x4 to_rotation4(const quaternion & lhs)
	{
		const float tx = 2.f * lhs.x;
		const float ty = 2.f * lhs.y;
		const float tz = 2.f * lhs.z;
		const float twx = tx * lhs.w;
		const float twy = ty * lhs.w;
		const float twz = tz * lhs.w;
		const float txx = tx * lhs.x;
		const float txy = ty * lhs.x;
		const float txz = tz * lhs.x;
		const float tyy = ty * lhs.y;
		const float tyz = tz * lhs.y;
		const float tzz = tz * lhs.z;

		return float4x4(
			1.f - (tyy + tzz), txy + twz, txz - twy, 0.f,
			txy - twz, 1.f - (txx + tzz), tyz + twx, 0.f,
			txz + twy, tyz - twx, 1.f - (txx + tyy), 0.f,
			0.f, 0.f, 0.f, 1.f
		);
	}

	inline mat128 FUSE_VECTOR_CALL to_rotation4(vec128 lhs)
	{
		// TODO
		return to_mat128(to_rotation4(to_quaternion(lhs)));
	}
	
	inline mat128 FUSE_VECTOR_CALL to_rotation3(vec128 lhs)
	{
		return to_rotation4(lhs);
	}

	void decompose_affine(const float4x4 & m, float3 * pScale, quaternion * pRotation, float3 * pTranslation);
	void FUSE_VECTOR_CALL decompose_affine(mat128 m, vec128 * pScale, vec128 * pRotation, vec128 * pTranslation);
}