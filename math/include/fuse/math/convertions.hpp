#pragma once

#include "vec128.hpp"
#include "vector3.hpp"
#include "vector4.hpp"
#include "mat128.hpp"
#include "matrix3x3.hpp"
#include "matrix4x4.hpp"
#include "vec128_impl.inl"
#include "mat128_impl.inl"

namespace fuse
{

	/* to_vec128 */

	inline vec128 FUSE_VECTOR_CALL to_vec128(vec128 lhs)
	{
		return lhs;
	}

	inline vec128 to_vec128(const float3 & lhs)
	{
		return vec128_load(lhs);
	}

	inline vec128 to_vec128(const float4 & lhs)
	{
		return vec128_load(lhs);
	}

	/* to_float3 */

	inline float3 FUSE_VECTOR_CALL to_float3(vec128 lhs)
	{
		float3 r;
		vec128_store(&r, lhs);
		return r;
	}

	inline float3 to_float3(const float3 & lhs)
	{
		return lhs;
	}

	inline float3 to_float3(const float4 & lhs)
	{
		return float3(lhs.x, lhs.y, lhs.z);
	}

	/* to_float4 */

	inline float4 FUSE_VECTOR_CALL to_float4(vec128 lhs)
	{
		float4 r;
		vec128_store(&r, lhs);
		return r;
	}

	inline float4 to_float4(const float3 & lhs)
	{
		return float4(lhs.x, lhs.y, lhs.z, 1);
	}

	inline float4 to_float4(const float4 & lhs)
	{
		return lhs;
	}

	/* to_mat128 */

	inline mat128 FUSE_VECTOR_CALL to_mat128(mat128 lhs)
	{
		return lhs;
	}

	inline mat128 to_mat128(const float3x3 & lhs)
	{
		return mat128_load(lhs);
	}

	inline mat128 to_mat128(const float4x4 & lhs)
	{
		return mat128_load(lhs);
	}

	/* to_float3x3 */

	inline float3x3 to_float3x3(const float4x4 & lhs)
	{
		return float3x3(
			lhs._00, lhs._01, lhs._02,
			lhs._10, lhs._11, lhs._12,
			lhs._20, lhs._21, lhs._22);
	}

	/* to_float4x4 */

	inline float4x4 to_float4x4(const float3x3 & lhs)
	{
		return float4x4(
			lhs._00, lhs._01, lhs._02, 0.f,
			lhs._10, lhs._11, lhs._12, 0.f,
			lhs._20, lhs._21, lhs._22, 0.f,
			0.f, 0.f, 0.f, 1.f);
	}

}