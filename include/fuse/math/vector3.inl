namespace fuse
{

	inline float3 cross(const float3 & lhs, const float3 & rhs)
	{
		return float3(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x);
	}

}