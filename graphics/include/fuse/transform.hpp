//#pragma once
//
//#include <fuse/core.hpp>
//#include <fuse/allocators.hpp>
//#include <fuse/math.hpp>
//
//namespace fuse
//{
//
//	class alignas(16) transform
//	{
//
//	public:
//
//		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)
//
//		inline transform(void)
//		{
//			m_matrix         = XMMatrixIdentity();
//			m_matrixUptodate = true;
//			m_translation    = vec128_zero();
//			m_rotation       = XMQuaternionIdentity();
//		}
//
//		inline const XMMATRIX & get_matrix(void) const
//		{
//			compute_matrix();
//			return m_matrix;
//		}
//
//		inline void set_translation(const vec128 & translation)
//		{
//			m_translation = translation;
//			m_matrixUptodate = false;
//		}
//
//		inline void set_rotation(const vec128 & rotation)
//		{
//			m_rotation = rotation;
//			m_matrixUptodate = false;
//		}
//
//		inline void set_scale(const vec128 & scale)
//		{
//			m_scale = scale;
//			m_matrixUptodate = false;
//		}
//
//		inline void set_matrix(const XMMATRIX & matrix)
//		{
//
//		}
//
//	private:
//
//		mutable XMMATRIX m_matrix;
//
//		vec128 m_translation;
//		vec128 m_rotation;
//		vec128 m_scale;
//		
//		mutable bool m_matrixUptodate;
//
//		inline void compute_matrix(void) const
//		{
//			if (!m_matrixUptodate)
//			{
//				m_matrix = XMMatrixAffineTransformation(m_scale, vec128_zero(), m_rotation, m_translation);
//				m_matrixUptodate = true;
//			}
//		}
//
//	public:
//
//		FUSE_PROPERTIES_BY_VALUE_READ_ONLY (
//			(translation, m_translation)
//			(rotation, m_rotation)
//			(scale, m_scale)
//		)
//
//	};
//
//}