#include "scalar_functions.hpp"

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
		return quaternion(q.w * invSquaredNorm, q.x * invSquaredNorm, q.y * invSquaredNorm, q.w * invSquaredNorm);
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

	inline float3x3 rotation3(const quaternion & lhs)
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

	inline float4x4 rotation4(const quaternion & lhs)
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

}