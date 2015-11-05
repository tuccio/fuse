#include <fuse/camera.hpp>

#include <boost/math/constants/constants.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(camera, 16)

camera::camera(void) :
	m_orientation(XMQuaternionIdentity()),
	m_position(XMVectorZero()),
	m_fovy(half_pi<float>()),
	m_znear(.1f),
	m_zfar(100.f),
	m_aspectRatio(1.f),
	m_viewMatrixDirty(true),
	m_projectionMatrixDirty(true) { }

void camera::update_view_matrix(void) const
{

	XMMATRIX rotation   = XMMatrixRotationQuaternion(m_orientation);
	m_viewMatrix = XMMatrixTranspose(rotation);

	XMVECTOR xyUU = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1Y, XM_PERMUTE_1Z, XM_PERMUTE_1W>(XMVector3Dot(rotation.r[0], m_position), XMVector3Dot(rotation.r[1], m_position));
	XMVECTOR UUzw = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(XMVector3Dot(rotation.r[2], m_position), XMVectorSet(0, 0, 0, -1));

	m_viewMatrix.r[3] = - XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_1Z, XM_PERMUTE_1W>(xyUU, UUzw);

	m_viewMatrixDirty = false;

}

void camera::update_projection_matrix(void) const
{

	float tanFovY  = std::tan(m_fovy * .5f);
	float yScale   = 1.f / tanFovY;
	float xScale   = yScale / m_aspectRatio;
	float invDepth = 1.f / (m_zfar - m_znear);

	float Q = m_zfar * invDepth;

	m_projectionMatrix = XMMatrixSet (
		xScale, 0, 0, 0,
		0, yScale, 0, 0,
		0, 0, Q, 1,
		0, 0, - Q * m_znear, 0
	);

	m_projectionMatrixDirty = false;

}

void camera::look_at(const XMFLOAT3 & eye, const XMFLOAT3 & up, const XMFLOAT3 & target)
{

	m_position = fuse::to_vector(eye);

	XMVECTOR z = XMVector3Normalize(m_position - to_vector(target));
	XMVECTOR x = XMVector3Normalize(XMVector3Cross(to_vector(up), z));
	XMVECTOR y = XMVector3Cross(z, x);

	XMMATRIX inverseRotation = XMMatrixIdentity();

	inverseRotation.r[0] = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(x, inverseRotation.r[0]);
	inverseRotation.r[1] = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(y, inverseRotation.r[1]);
	inverseRotation.r[2] = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(z, inverseRotation.r[2]);

	//XMMATRIX rotation = XMMatrixTranspose(inverseRotation);

	//m_orientation = XMQuaternionRotationMatrix(rotation);
	m_orientation = XMQuaternionConjugate(XMQuaternionRotationMatrix(inverseRotation));

	m_viewMatrixDirty = true;

}

void camera::set_projection(float fovy, float znear, float zfar)
{
	m_fovy  = fovy;
	m_znear = znear;
	m_zfar  = zfar;
	m_projectionMatrixDirty = true;
}

XMMATRIX camera::get_view_matrix(void) const
{

	if (m_viewMatrixDirty)
	{
		update_view_matrix();
	}

	return m_viewMatrix;

}

XMMATRIX camera::get_projection_matrix(void) const
{

	if (m_projectionMatrixDirty)
	{
		update_projection_matrix();
	}

	return m_projectionMatrix;

}

void camera::set_position(const XMVECTOR & position)
{
	m_position = position;
	m_viewMatrixDirty = true;
}

void camera::set_orientation(const XMVECTOR & orientation)
{
	m_orientation = orientation;
	m_viewMatrixDirty = true;
}

void camera::set_aspect_ratio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
	m_projectionMatrixDirty = true;
}

void camera::set_fovy(float fovy)
{
	m_fovy = fovy;
	m_projectionMatrixDirty = true;
}

void camera::set_znear(float znear)
{
	m_znear = znear;
	m_projectionMatrixDirty = true;
}

void camera::set_zfar(float zfar)
{
	m_zfar = zfar;
	m_projectionMatrixDirty = true;
}

float camera::get_fovx(void) const
{
	return 2 * std::atan(std::tan(m_fovy * .5f) * m_aspectRatio);
}

void camera::set_fovx(float fovx)
{
	m_fovy = 2 * std::atan(std::tan(fovx * .5f) / m_aspectRatio);
}