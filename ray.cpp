#include <fuse/geometry/ray.hpp>

using namespace fuse;

XMVECTOR ray::evaluate(float t)
{
	return XMVectorAdd(m_origin, XMVectorScale(m_direction, t));
}