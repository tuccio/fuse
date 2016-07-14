#pragma once

namespace fuse
{

	inline float3 cross(const float3 & lhs, const float3 & rhs)
	{
		return float3(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x);
	}

	inline matrix<float, 3, 1> cross(const matrix<float, 3, 1> & lhs, const matrix<float, 3, 1> & rhs)
	{
		float3 r = cross(reinterpret_cast<const float3&>(lhs), reinterpret_cast<const float3&>(rhs));
		return transpose(r);
	}

	inline float4x4 to_translation4(const float3 & lhs)
	{
		return float4x4(
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			lhs.x, lhs.y, lhs.z, 1.f);
	}

	inline float4x4 to_scale4(const float3 & lhs)
	{
		return float4x4(
			lhs.x, 0.f, 0.f, 0.f,
			0.f, lhs.y, 0.f, 0.f,
			0.f, 0.f, lhs.z, 0.f,
			0.f, 0.f, 0.f, 1.f);
	}

}