//#pragma once
//
//#include <fuse/allocators.hpp>
//#include <fuse/math.hpp>
//#include <fuse/properties_macros.hpp>
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
//			m_translation    = XMVectorZero();
//			m_rotation       = XMQuaternionIdentity();
//		}
//
//		inline const XMMATRIX & get_matrix(void) const
//		{
//			compute_matrix();
//			return m_matrix;
//		}
//
//		inline void set_translation(const XMVECTOR & translation)
//		{
//			m_translation = translation;
//			m_matrixUptodate = false;
//		}
//
//		inline void set_rotation(const XMVECTOR & rotation)
//		{
//			m_rotation = rotation;
//			m_matrixUptodate = false;
//		}
//
//		inline void set_scale(const XMVECTOR & scale)
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
//		XMVECTOR m_translation;
//		XMVECTOR m_rotation;
//		XMVECTOR m_scale;
//		
//		mutable bool m_matrixUptodate;
//
//		inline void compute_matrix(void) const
//		{
//			if (!m_matrixUptodate)
//			{
//				m_matrix = XMMatrixAffineTransformation(m_scale, XMVectorZero(), m_rotation, m_translation);
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