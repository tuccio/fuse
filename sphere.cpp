#include <fuse/geometry/sphere.hpp>

using namespace fuse;

sphere::sphere(const XMFLOAT3 & center, float radius) :
	m_sphere(XMVectorSet(center.x, center.y, center.z, radius)) { }

sphere::sphere(const XMVECTOR & sphere) :
	m_sphere(sphere) { }

void sphere::set_center(const XMVECTOR & center)
{
	m_sphere = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(center, m_sphere);
}

float sphere::get_radius(void) const
{
	return XMVectorGetW(m_sphere);
}

void sphere::set_radius(float radius)
{
	m_sphere = XMVectorSetW(m_sphere, radius);
}