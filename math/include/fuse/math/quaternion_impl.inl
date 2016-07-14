#include "scalar_functions.hpp"

#include <cmath>
#include <algorithm>

namespace fuse
{

	/* Arithmetic */

	inline quaternion operator+ (const quaternion & lhs, const quaternion & rhs)
	{
		return quaternion(
			lhs.w + rhs.w,
			lhs.w + rhs.x,
			lhs.w + rhs.y,
			lhs.w + rhs.z);
	}

	inline quaternion operator- (const quaternion & lhs, const quaternion & rhs)
	{
		return quaternion(
			lhs.w - rhs.w,
			lhs.w - rhs.x,
			lhs.w - rhs.y,
			lhs.w - rhs.z);
	}

	inline quaternion operator* (const quaternion & lhs, const quaternion & rhs)
	{
		return quaternion(
			lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z,
			lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x);
	}

	inline quaternion conjugate(const quaternion & q)
	{
		return quaternion(q.w, -q.x, -q.y, -q.z);
	}

	inline quaternion inverse(const quaternion & q)
	{
		float invSquaredNorm = 1.f / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
		return quaternion(q.w * invSquaredNorm, q.x * invSquaredNorm, q.y * invSquaredNorm, q.z * invSquaredNorm);
	}

	inline float norm(const quaternion & q)
	{
		return square_root(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	inline quaternion normalize(const quaternion & q)
	{
		float invNorm = inverse_square_root(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
		return quaternion(q.w * invNorm, q.x * invNorm, q.y * invNorm, q.z * invNorm);
	}

	inline float3 transform(const float3 & lhs, const quaternion & rhs)
	{
		/*quaternion pq(0, lhs.x, lhs.y, lhs.z);
		quaternion r = rhs * pq * conjugate(rhs);
		return float3(r.x, r.y, r.z);*/

		/*float3 u(rhs.x, rhs.y, rhs.z);

		float s = rhs.w;

		return 2.f * dot(u, lhs) * u
			+ (s * s - dot(u, u)) * lhs
			+ 2.f * s * cross(u, lhs);*/

		auto u = reinterpret_cast<const float3 &>(rhs.x);
		
		float3 uv = 2.f * cross(u, lhs);
		return lhs + rhs.w * uv + cross(u, uv);
	}

	inline float4 transform(const float4 & lhs, const quaternion & rhs)
	{
		float3 r = transform(reinterpret_cast<const float3 &>(lhs), rhs);
		return float4(r.x, r.y, r.z, 1.f);
	}

	inline quaternion to_quaternion(const float3x3 & lhs)
	{
		quaternion q;
		q.w = square_root(std::max(0.f, 1.f + lhs._00 + lhs._11 + lhs._22)) * .5f;
		q.x = square_root(std::max(0.f, 1.f + lhs._00 - lhs._11 - lhs._22)) * .5f;
		q.y = square_root(std::max(0.f, 1.f - lhs._00 + lhs._11 - lhs._22)) * .5f;
		q.z = square_root(std::max(0.f, 1.f - lhs._00 - lhs._11 + lhs._22)) * .5f;
		q.x = std::copysign(q.x, lhs._21 - lhs._12);
		q.y = std::copysign(q.y, lhs._02 - lhs._20);
		q.z = std::copysign(q.z, lhs._10 - lhs._01);
		return q;
	}

	inline quaternion to_quaternion(const float4x4 & lhs)
	{
		quaternion q;
		q.w = square_root(std::max(0.f, 1.f + lhs._00 + lhs._11 + lhs._22)) * .5f;
		q.x = square_root(std::max(0.f, 1.f + lhs._00 - lhs._11 - lhs._22)) * .5f;
		q.y = square_root(std::max(0.f, 1.f - lhs._00 + lhs._11 - lhs._22)) * .5f;
		q.z = square_root(std::max(0.f, 1.f - lhs._00 - lhs._11 + lhs._22)) * .5f;
		q.x = std::copysign(q.x, lhs._21 - lhs._12);
		q.y = std::copysign(q.y, lhs._02 - lhs._20);
		q.z = std::copysign(q.z, lhs._10 - lhs._01);
		return q;
	}

}