#include <fuse/geometry/aabb.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(aabb, 16)

aabb fuse::operator+ (const aabb & a, const aabb & b)
{
	return aabb::from_min_max(
		XMVectorMin(a.get_min(), b.get_min()),
		XMVectorMax(a.get_max(), b.get_max())
	);
}

aabb fuse::operator^ (const aabb & a, const aabb & b)
{
	return aabb::from_min_max(
		XMVectorMax(a.get_min(), b.get_min()),
		XMVectorMin(a.get_max(), b.get_max())
	);
}

std::array<XMVECTOR, 8> aabb::get_corners(void) const
{

	std::array<XMVECTOR, 8> v;

	for (int i = 0; i < 8; i++)
	{
		XMVECTOR sign = XMVectorSetInt((i & 4) << 29, (i & 2) << 30, (i & 1) << 31,	0);
		v[i] = XMVectorAdd(m_center, XMVectorOrInt(sign, m_halfExtents));
	}

	return v;

}