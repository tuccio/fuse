#include <fuse/geometry/aabb.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(aabb, 16)

aabb aabb::from_min_max(const XMVECTOR & min, const XMVECTOR & max)
{
	aabb box;
	box.m_center = XMVectorAdd(min, max) * .5f;
	box.m_halfExtents = XMVectorSubtract(max, min) * .5f;
	return box;
}

aabb aabb::from_center_half_extents(const XMVECTOR & center, const XMVECTOR & halfExtents)
{
	aabb box;
	box.m_center = center;
	box.m_halfExtents = halfExtents;
	return box;
}

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