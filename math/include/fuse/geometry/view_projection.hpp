#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	float4x4 look_at_lh(const float3 & position, const float3 & target, const float3 & up);
	float4x4 look_at_rh(const float3 & position, const float3 & target, const float3 & up);

	mat128 FUSE_VECTOR_CALL look_at_lh(vec128 position, vec128 target, vec128 up);
	mat128 FUSE_VECTOR_CALL look_at_rh(vec128 position, vec128 target, vec128 up);

	float4x4 ortho_lh(float left, float right, float bottom, float top, float znear, float zfar);
	float4x4 ortho_rh(float left, float right, float bottom, float top, float znear, float zfar);

	vec128 FUSE_VECTOR_CALL ortho_lh(vec128 lbn, vec128 rtf);
	vec128 FUSE_VECTOR_CALL ortho_rh(vec128 lbn, vec128 rtf);

}