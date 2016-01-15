#include <fuse/geometry/ray.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(ray, 16)

XMVECTOR ray::evaluate(float t)
{
	return XMVectorAdd(m_origin, XMVectorScale(m_direction, t));
}