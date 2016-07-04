#include <fuse/geometry/plane.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(plane, 16)

plane::plane(const XMVECTOR & plane) :
	m_plane(plane) {}

plane::plane(float a, float b, float c, float d) :
	m_plane(XMVectorSet(a, b, c, d)) {}

plane::plane(const XMFLOAT3 & normal, const XMFLOAT3 & point) :
	m_plane(XMPlaneFromPointNormal(to_vector(point), to_vector(normal))) {}

plane plane::flip(void) const
{
	return plane(XMVectorXorInt(m_plane, XMVectorSet(-0.f, -0.f, -0.f, -0.f)));
}

plane plane::normalize(void) const
{
	return plane(XMPlaneNormalize(m_plane));
}