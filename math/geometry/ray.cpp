#include <fuse/geometry/ray.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(ray, 16)

vec128 ray::evaluate(float t) const
{
	return m_origin + (t * m_direction);
}