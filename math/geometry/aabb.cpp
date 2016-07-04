#include <fuse/geometry/aabb.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(aabb, 16)

aabb fuse::operator+ (const aabb & a, const aabb & b)
{
	return aabb::from_min_max(
		vec128_min(a.get_min(), b.get_min()),
		vec128_max(a.get_max(), b.get_max())
	);
}

aabb fuse::operator^ (const aabb & a, const aabb & b)
{
	return aabb::from_min_max(
		vec128_max(a.get_min(), b.get_min()),
		vec128_min(a.get_max(), b.get_max())
	);
}

std::array<XMVECTOR, 8> aabb::get_corners(void) const
{

	std::array<vec128, 8> v;

	for (int i = 0; i < 8; i++)
	{
		vec128 sign  = vec128_i32((i & 1) << 31, (i & 2) << 30, (i & 4) << 29, 0);
		vec128 delta = vec128_or(sign, m_halfExtents);
		v[i] = m_center + delta;
	}

	return v;

}