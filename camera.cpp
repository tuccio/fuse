#include <fuse/camera.hpp>

#include <boost/math/constants/constants.hpp>

using namespace fuse;

camera::camera(void) :
	m_orientation(1, 0, 0, 0),
	m_position(0, 0, 0),
	m_fovy(half_pi<float>()),
	m_znear(.1f),
	m_zfar(100.f),
	m_aspectRatio(1.f),
	m_viewMatrixDirty(true),
	m_projectionMatrixDirty(true) { }

void camera::update_world_matrix(void) const
{
	float4x4 rotation = to_rotation4(m_orientation);
	m_worldMatrix = rotation * to_translation4(m_position);
	m_worldMatrixDirty = false;
}

void camera::update_view_matrix(void) const
{
	float4x4 rotation = to_rotation4(m_orientation);
	
	m_viewMatrix = transpose(rotation);

	m_viewMatrix._30 = -dot(float3(rotation._00, rotation._01, rotation._02), m_position);
	m_viewMatrix._31 = -dot(float3(rotation._10, rotation._11, rotation._12), m_position);
	m_viewMatrix._32 = -dot(float3(rotation._20, rotation._21, rotation._22), m_position);

	m_viewMatrixDirty = false;
}

void camera::update_projection_matrix(void) const
{
	float tanFovY  = std::tan(m_fovy * .5f);
	float yScale   = 1.f / tanFovY;
	float xScale   = yScale / m_aspectRatio;
	float invDepth = 1.f / (m_zfar - m_znear);

	float Q = m_zfar * invDepth;

	m_projectionMatrix = float4x4(
		xScale, 0, 0, 0,
		0, yScale, 0, 0,
		0, 0, Q, 1,
		0, 0, - Q * m_znear, 0
	);

	m_projectionMatrixDirty = false;
}

void camera::look_at(const float3 & eye, const float3 & up, const float3 & target)
{
	m_position = eye;

	float3 z = normalize(m_position - target);
	float3 x = normalize(cross(up, z));
	float3 y = cross(z, x);

	float4x4 inverseRotation(
		x.x, x.y, x.z, 0.f,
		y.x, y.y, y.z, 0.f,
		z.x, z.y, z.z, 0.f,
		0.f, 0.f, 0.f, 1.f
	);

	m_orientation = conjugate(to_quaternion(inverseRotation));

	m_viewMatrixDirty = true;
	m_worldMatrixDirty = true;
}

void camera::set_projection(float fovy, float znear, float zfar)
{
	m_fovy  = fovy;
	m_znear = znear;
	m_zfar  = zfar;
	m_projectionMatrixDirty = true;
}

const float4x4 & camera::get_world_matrix(void) const
{
	if (m_worldMatrixDirty)
	{
		update_world_matrix();
	}

	return m_worldMatrix;
}

const float4x4 & camera::get_view_matrix(void) const
{
	if (m_viewMatrixDirty)
	{
		update_view_matrix();
	}

	return m_viewMatrix;
}

const float4x4 & camera::get_projection_matrix(void) const
{
	if (m_projectionMatrixDirty)
	{
		update_projection_matrix();
	}

	return m_projectionMatrix;
}

void camera::set_world_matrix(const float4x4 & matrix)
{
	throw;
}

void camera::set_view_matrix(const float4x4 & matrix)
{
	throw;
}

void camera::set_projection_matrix(const float4x4 & matrix)
{
	throw;
}

frustum camera::get_frustum(void) const
{
	XMMATRIX v, p;

	p = XMLoadFloat4x4((const XMFLOAT4X4*)&transpose(get_projection_matrix()));
	v = XMLoadFloat4x4((const XMFLOAT4X4*)&transpose(get_view_matrix()));

	return frustum(p, v);
}

void camera::set_position(const float3 & position)
{
	m_position = position;
	m_worldMatrixDirty = true;
	m_viewMatrixDirty = true;
}

void camera::set_orientation(const quaternion & orientation)
{
	m_orientation = orientation;
	m_worldMatrixDirty = true;
	m_viewMatrixDirty = true;
}

void camera::set_aspect_ratio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
	m_worldMatrixDirty = true;
	m_projectionMatrixDirty = true;
}

void camera::set_fovy(float fovy)
{
	m_fovy = fovy;
	m_worldMatrixDirty = true;
	m_projectionMatrixDirty = true;
}

void camera::set_znear(float znear)
{
	m_znear = znear;
	m_worldMatrixDirty = true;
	m_projectionMatrixDirty = true;
}

void camera::set_zfar(float zfar)
{
	m_zfar = zfar;
	m_worldMatrixDirty = true;
	m_projectionMatrixDirty = true;
}

float camera::get_fovx(void) const
{
	return 2 * std::atan(std::tan(m_fovy * .5f) * m_aspectRatio);
}

void camera::set_fovx(float fovx)
{
	m_fovy = 2 * std::atan(std::tan(fovx * .5f) / m_aspectRatio);
	m_worldMatrixDirty = true;
	m_projectionMatrixDirty = true;
}

float camera::get_fovy_deg(void) const
{
	return rad_to_deg(get_fovy());
}

float camera::get_fovx_deg(void) const
{
	return rad_to_deg(get_fovx());
}

void camera::set_fovy_deg(float fovy)
{
	set_fovy(deg_to_rad(fovy));
}

void camera::set_fovx_deg(float fovx)
{
	set_fovx(deg_to_rad(fovx));
}