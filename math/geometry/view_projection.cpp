#pragma once

#include <fuse/geometry/view_projection.hpp>

namespace fuse
{

	float4x4 look_at_lh(const float3 & position, const float3 & target, const float3 & up)
	{
		float3 z = normalize(target - position);
		float3 x = normalize(cross(up, z));
		float3 y = cross(z, x);
		
		return float4x4(
			x.x, y.x, z.x, 0.f,
			x.y, y.y, z.y, 0.f,
			x.z, y.z, z.z, 0.f,
			-dot(x, position), -dot(y, position), -dot(z, position), 1.f
		);
	}

	float4x4 look_at_rh(const float3 & position, const float3 & target, const float3 & up)
	{
		float3 z = normalize(position - target);
		float3 x = normalize(cross(up, z));
		float3 y = cross(z, x);

		return float4x4(
			x.x, y.x, z.x, 0.f,
			x.y, y.y, z.y, 0.f,
			x.z, y.z, z.z, 0.f,
			-dot(x, position), -dot(y, position), -dot(z, position), 1.f
		);
	}

	mat128 FUSE_VECTOR_CALL look_at_lh(vec128 position, vec128 target, vec128 up)
	{
		vec128 z = vec128_normalize3(target - position);
		vec128 x = vec128_normalize3(vec128_cross(up, z));
		vec128 y = vec128_cross(z, x);

		mat128 r;
		r.c[0] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(x, -vec128_dot3(x, position));
		r.c[1] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(y, -vec128_dot3(y, position));
		r.c[2] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(z, -vec128_dot3(z, position));
		r.c[3] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_zero(), vec128_one());
		return r;
	}

	mat128 FUSE_VECTOR_CALL look_at_rh(vec128 position, vec128 target, vec128 up)
	{
		vec128 z = vec128_normalize3(position - target);
		vec128 x = vec128_normalize3(vec128_cross(up, z));
		vec128 y = vec128_cross(z, x);

		mat128 r;
		r.c[0] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(x, -vec128_dot3(x, position));
		r.c[1] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(y, -vec128_dot3(y, position));
		r.c[2] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(z, -vec128_dot3(z, position));
		r.c[3] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(vec128_zero(), vec128_one());
		return r;
	}

	float4x4 ortho_lh(float left, float right, float bottom, float top, float znear, float zfar)
	{
		float invW = 1.f / (right - left);
		float invH = 1.f / (top - bottom);
		float invD = 1.f / (zfar - znear);

		return float4x4(
			2.f * invW, 0.f, 0.f, 0.f,
			0.f, 2.f * invH, 0.f, 0.f,
			0.f, 0.f, invD, 0.f,
			- (left + right) * invW, - (top + bottom) * invH, - znear * invD, 1.f
		);
	}

	float4x4 ortho_rh(float left, float right, float bottom, float top, float znear, float zfar)
	{
		float invW = 1.f / (right - left);
		float invH = 1.f / (top - bottom);
		float invD = 1.f / (zfar - znear);

		return float4x4(
			2.f * invW, 0.f, 0.f, 0.f,
			0.f, 2.f * invH, 0.f, 0.f,
			0.f, 0.f, -invD, 0.f,
			-(left + right) * invW, -(top + bottom) * invH, -znear * invD, 1.f
		);
	}

}