#include <fuse/geometry/plane.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(plane, 16)

plane::plane(const vec128 & plane) :
	m_plane(plane) {}

plane::plane(float a, float b, float c, float d) :
	m_plane(vec128_set(a, b, c, d)) {}

plane::plane(const float3 & normal, const float3 & point)
{
	vec128 N = vec128_load(normal);
	vec128 x = vec128_load(point);
	vec128 w = vec128_negate(vec128_dot3(N, x));
	m_plane = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(N, w);
}

plane::plane(const float4 & plane) :
	m_plane(vec128_load(plane)) {}

plane plane::flip(void) const
{
	return plane(vec128_negate(m_plane));
}

plane plane::normalize(void) const
{
	vec128 norm = vec128_dot3(m_plane, m_plane);
	vec128 invSqrtNorm = vec128_invsqrt(norm);
	return plane(vec128_mul(m_plane, invSqrtNorm));
}

vec128 FUSE_VECTOR_CALL plane::dot(vec128 x) const
{
	return vec128_dot4(vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(x, vec128_one()), m_plane);
}