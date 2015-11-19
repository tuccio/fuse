#include <fuse/geometry/frustum.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(frustum, 16)

frustum::frustum(const XMMATRIX & viewProjection)
{

	XMMATRIX viewProjectionT = XMMatrixTranspose(viewProjection);

	m_planes[FUSE_FRUSTUM_PLANE_NEAR].set_plane_vector(viewProjectionT.r[2]);
	m_planes[FUSE_FRUSTUM_PLANE_FAR].set_plane_vector(viewProjectionT.r[3] - viewProjectionT.r[2]);

	m_planes[FUSE_FRUSTUM_PLANE_TOP].set_plane_vector(viewProjectionT.r[3] - viewProjectionT.r[1]);
	m_planes[FUSE_FRUSTUM_PLANE_BOTTOM].set_plane_vector(viewProjectionT.r[3] + viewProjectionT.r[1]);

	m_planes[FUSE_FRUSTUM_PLANE_LEFT].set_plane_vector(viewProjectionT.r[3] + viewProjectionT.r[0]);
	m_planes[FUSE_FRUSTUM_PLANE_RIGHT].set_plane_vector(viewProjectionT.r[3] - viewProjectionT.r[0]);

}