#include <fuse/geometry/sphere.hpp>

using namespace fuse;

sphere::sphere(const float3 & center, float radius) :
	m_sphere(vec128_set(center.x, center.y, center.z, radius)) { }

sphere::sphere(const vec128 & center, const vec128 & radius)
{
	m_sphere = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(center, radius);
}

sphere::sphere(const vec128 & sphere) :
	m_sphere(sphere) { }

void FUSE_VECTOR_CALL sphere::set_center(vec128 center)
{
	m_sphere = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(center, m_sphere);
}

vec128 sphere::get_radius(void) const
{
	return vec128_splat<FUSE_W>(m_sphere);
}

void FUSE_VECTOR_CALL sphere::set_radius(vec128 radius)
{
	m_sphere = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(m_sphere, radius);
}