#include <fuse/geometry/frustum.hpp>
#include <fuse/geometry/cramer.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(frustum, 16)

frustum::frustum(const XMMATRIX & projection, const XMMATRIX & view)
{

	/*XMMATRIX viewProjectionT = XMMatrixTranspose(view * projection);

	m_planes[FUSE_FRUSTUM_PLANE_NEAR]   = plane(viewProjectionT.r[2]).normalize();
	m_planes[FUSE_FRUSTUM_PLANE_FAR]    = plane(viewProjectionT.r[3] - viewProjectionT.r[2]).normalize();

	m_planes[FUSE_FRUSTUM_PLANE_TOP]    = plane(viewProjectionT.r[3] - viewProjectionT.r[1]).normalize();
	m_planes[FUSE_FRUSTUM_PLANE_BOTTOM] = plane(viewProjectionT.r[3] + viewProjectionT.r[1]).normalize();

	m_planes[FUSE_FRUSTUM_PLANE_LEFT]   = plane(viewProjectionT.r[3] + viewProjectionT.r[0]).normalize();
	m_planes[FUSE_FRUSTUM_PLANE_RIGHT]  = plane(viewProjectionT.r[3] - viewProjectionT.r[0]).normalize();*/

	XMMATRIX viewProjectionT = XMMatrixTranspose(view * projection);

	m_planes[FUSE_FRUSTUM_PLANE_NEAR]   = plane(viewProjectionT.r[2]).flip().normalize();
	m_planes[FUSE_FRUSTUM_PLANE_FAR]    = plane(viewProjectionT.r[3] - viewProjectionT.r[2]).flip().normalize();

	m_planes[FUSE_FRUSTUM_PLANE_TOP]    = plane(viewProjectionT.r[3] - viewProjectionT.r[1]).flip().normalize();
	m_planes[FUSE_FRUSTUM_PLANE_BOTTOM] = plane(viewProjectionT.r[3] + viewProjectionT.r[1]).flip().normalize();

	m_planes[FUSE_FRUSTUM_PLANE_LEFT]   = plane(viewProjectionT.r[3] + viewProjectionT.r[0]).flip().normalize();
	m_planes[FUSE_FRUSTUM_PLANE_RIGHT]  = plane(viewProjectionT.r[3] - viewProjectionT.r[0]).flip().normalize();

}

static inline vec128 plane_intersection(const plane & p1, const plane & p2, const plane & p3)
{
	const XMVECTOR & p1v = p1.get_plane_vector();
	const XMVECTOR & p2v = p2.get_plane_vector();
	const XMVECTOR & p3v = p3.get_plane_vector();

	mat128 A, At;
	vec128 b;

	At.c[0] = p1v;
	At.c[1] = p2v;
	At.c[2] = p3v;

	A = mat128_transpose(At);
	b = vec128_neg(vec128_shuffle<FUSE_X0, FUSE_Y0, FUSE_W1, FUSE_W1>(vec128_permute<FUSE_W0, FUSE_W1, FUSE_Z0, FUSE_W0>(p1v, p2v), p3v));

	/*XMStoreFloat3((XMFLOAT3*)A.m[0], p1v);
	XMStoreFloat3((XMFLOAT3*)A.m[1], p2v);
	XMStoreFloat3((XMFLOAT3*)A.m[2], p3v);

	b.x = - XMVectorGetW(p1v);
	b.y = - XMVectorGetW(p2v);
	b.z = - XMVectorGetW(p3v);*/

	return cramer3(A, b);
}

std::array<vec128, 8> frustum::get_corners(void) const
{

	std::array<vec128, 8> corners;

	corners[FUSE_FRUSTUM_NEAR_BOTTOM_RIGHT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_NEAR],
		m_planes[FUSE_FRUSTUM_PLANE_BOTTOM],
		m_planes[FUSE_FRUSTUM_PLANE_RIGHT]);

	corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_NEAR],
		m_planes[FUSE_FRUSTUM_PLANE_BOTTOM],
		m_planes[FUSE_FRUSTUM_PLANE_LEFT]);

	corners[FUSE_FRUSTUM_NEAR_TOP_RIGHT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_NEAR],
		m_planes[FUSE_FRUSTUM_PLANE_TOP],
		m_planes[FUSE_FRUSTUM_PLANE_RIGHT]);

	corners[FUSE_FRUSTUM_NEAR_TOP_LEFT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_NEAR],
		m_planes[FUSE_FRUSTUM_PLANE_TOP],
		m_planes[FUSE_FRUSTUM_PLANE_LEFT]);

	corners[FUSE_FRUSTUM_FAR_BOTTOM_RIGHT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_FAR],
		m_planes[FUSE_FRUSTUM_PLANE_BOTTOM],
		m_planes[FUSE_FRUSTUM_PLANE_RIGHT]);

	corners[FUSE_FRUSTUM_FAR_BOTTOM_LEFT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_FAR],
		m_planes[FUSE_FRUSTUM_PLANE_BOTTOM],
		m_planes[FUSE_FRUSTUM_PLANE_LEFT]);

	corners[FUSE_FRUSTUM_FAR_TOP_RIGHT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_FAR],
		m_planes[FUSE_FRUSTUM_PLANE_TOP],
		m_planes[FUSE_FRUSTUM_PLANE_RIGHT]);

	corners[FUSE_FRUSTUM_FAR_TOP_LEFT] = plane_intersection(
		m_planes[FUSE_FRUSTUM_PLANE_FAR],
		m_planes[FUSE_FRUSTUM_PLANE_TOP],
		m_planes[FUSE_FRUSTUM_PLANE_LEFT]);

	return corners;

}