#pragma once

#include <fuse/core.hpp>
#include <fuse/math.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/geometry.hpp>

#include <fuse/math/math.hpp>

namespace fuse
{

	class camera
	{

	public:

		camera(void);

		void look_at(const float3 & eye, const float3 & up, const float3 & target);

		inline float3 forward(void) const
		{
			return transform(float3(0, 0, 1), m_orientation);
		}

		inline float3 up(void) const
		{
			return transform(float3(0, 1, 0), m_orientation);
		}

		inline float3 right(void) const
		{
			return transform(float3(1, 0, 0), m_orientation);
		}

		inline float3 backward(void) const
		{
			return -forward();
		}

		inline float3 down(void) const
		{
			return -up();
		}

		inline float3 left(void) const
		{
			return -right();
		}

		const float4x4 & get_world_matrix(void) const;
		const float4x4 & get_view_matrix(void) const;
		const float4x4 & get_projection_matrix(void) const;

		void set_world_matrix(const float4x4 & matrix);
		void set_view_matrix(const float4x4 & matrix);
		void set_projection_matrix(const float4x4 & matrix);

		float get_fovx(void) const;

		void set_projection(float fovy, float znear, float zfar);

		void set_position(const float3 & position);
		void set_orientation(const quaternion & orientation);
		void set_aspect_ratio(float aspectRatio);
		void set_fovy(float fovy);
		void set_fovx(float fovx);
		void set_znear(float znear);
		void set_zfar(float zfar);

		float get_fovy_deg(void) const;
		float get_fovx_deg(void) const;

		void set_fovy_deg(float fovy);
		void set_fovx_deg(float fovx);

		frustum get_frustum(void) const;

	private:

		float3     m_position;
		quaternion m_orientation;

		mutable float4x4 m_worldMatrix;
		mutable float4x4 m_viewMatrix;
		mutable float4x4 m_projectionMatrix;

		float m_aspectRatio;
		float m_fovy;

		float m_znear;
		float m_zfar;

		mutable bool m_worldMatrixDirty;
		mutable bool m_viewMatrixDirty;
		mutable bool m_projectionMatrixDirty;

		void update_world_matrix(void) const;
		void update_view_matrix(void) const;
		void update_projection_matrix(void) const;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(position,     m_position)
			(orientation,  m_orientation)
		)

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(aspect_ratio, m_aspectRatio)
			(fovy,         m_fovy)
			(znear,        m_znear)
			(zfar,         m_zfar)

		)
	};

}