#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>
#include <fuse/properties_macros.hpp>

namespace fuse
{

	class alignas(16) camera
	{

	public:

		camera(void);

		void look_at(const XMFLOAT3 & eye, const XMFLOAT3 & up, const XMFLOAT3 & target);

		inline XMVECTOR forward(void) const
		{
			static XMVECTOR k = XMLoadFloat3(&XMFLOAT3(0, 0, 1));
			return XMVector3Rotate(k, m_orientation);
		}

		inline XMVECTOR up(void) const
		{
			static XMVECTOR j = XMLoadFloat3(&XMFLOAT3(0, 1, 0));
			return XMVector3Rotate(j, m_orientation);
		}

		inline XMVECTOR right(void) const
		{
			static XMVECTOR i = XMLoadFloat3(&XMFLOAT3(1, 0, 0));
			return XMVector3Rotate(i, m_orientation);
		}

		inline XMVECTOR backward(void) const
		{
			return -forward();
		}

		inline XMVECTOR down(void) const
		{
			return -up();
		}

		inline XMVECTOR left(void) const
		{
			return -left();
		}

		XMMATRIX get_view_matrix(void) const;
		XMMATRIX get_projection_matrix(void) const;

		float get_fovx(void) const;

		void set_projection(float fovy, float znear, float zfar);

		void set_position(const XMVECTOR & position);
		void set_orientation(const XMVECTOR & orientation);
		void set_aspect_ratio(float aspectRatio);
		void set_fovy(float fovy);
		void set_fovx(float fovx);
		void set_znear(float znear);
		void set_zfar(float zfar);

	private:

		XMVECTOR m_position;
		XMVECTOR m_orientation;

		mutable XMMATRIX m_viewMatrix;
		mutable XMMATRIX m_projectionMatrix;

		float    m_aspectRatio;
		float    m_fovy;

		float    m_znear;
		float    m_zfar;

		mutable bool m_viewMatrixDirty;
		mutable bool m_projectionMatrixDirty;

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

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

	};

}