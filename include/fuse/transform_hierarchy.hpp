#pragma once

#include <fuse/math.hpp>
#include <fuse/transform.hpp>

namespace fuse
{

	template <typename T>
	class alignas(16) transform_hierarchy;

	template <typename TransformHierarchyNode>
	TransformHierarchyNode * get_transform_hierarchy_parent(transform_hierarchy<TransformHierarchyNode> * node);

	template <typename TransformHierarchyNode>
	typename TransformHierarchyNode::children_iterator get_transform_hierarchy_children_begin(transform_hierarchy<TransformHierarchyNode> * node);

	template <typename TransformHierarchyNode>
	typename TransformHierarchyNode::children_iterator get_transform_hierarchy_children_end(transform_hierarchy<TransformHierarchyNode> * node);

	template <typename T>
	class alignas(16) transform_hierarchy
	{

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		transform_hierarchy(void) :
			m_globalMatrixUptodate(false),
			m_globalTranslationRotationMatrixUptodate(false),
			m_localMatricesUptodate(false),
			m_localRotation(quat128_identity()),
			m_localScale(vec128_one()),
			m_localTranslation(vec128_zero()),
			m_localMatrix(mat128_identity()),
			m_globalMatrix(mat128_identity()),
			m_globalTranslationRotationMatrix(mat128_identity()),
			m_moved(true) {}

	public:

		const mat128 & get_global_matrix(void)
		{
			update_global_matrix();
			return m_globalMatrix;
		}

		const mat128 & get_local_matrix(void)
		{
			update_local_matrices();
			return m_localMatrix;
		}

		inline void set_local_translation(const float3 & t)
		{
			set_local_rotation(to_vec128(t));
		}

		inline void set_local_rotation(const quaternion & q)
		{
			set_local_rotation(to_quat128(q));
		}

		inline void set_local_scale(const float3 & s)
		{
			set_local_scale(to_vec128(s));
		}

		inline void FUSE_VECTOR_CALL set_local_translation(vec128 t)
		{
			m_localTranslation = t;
			m_localMatricesUptodate = false;
			m_globalMatrixUptodate  = false;
			m_moved = true;
		}

		inline void FUSE_VECTOR_CALL set_local_rotation(vec128 r)
		{
			m_localRotation = r;
			m_localMatricesUptodate = false;
			m_globalMatrixUptodate  = false;
			m_moved = true;
		}

		inline void FUSE_VECTOR_CALL set_local_scale(vec128 s)
		{
			m_localScale = s;
			m_localMatricesUptodate = false;
			m_globalMatrixUptodate  = false;
			m_moved = true;
		}

		vec128 get_local_translation(void) const
		{
			return m_localTranslation;
		}

		vec128 set_local_rotation(void) const
		{
			return m_localRotation;
		}

		vec128 set_local_scale(void) const
		{
			return m_localScale;
		}

	protected:

		const mat128 & get_global_translation_rotation_matrix(void)
		{
			update_global_rotation_translation_matrix();
			return m_globalTranslationRotationMatrix;
		}

		void notify_local_change(void)
		{
			notify_global_change();
			m_localMatricesUptodate  = false;
			m_globalMatrixUptodate = false;
			m_moved = true;
		}

		void notify_global_change(void)
		{
			for (auto it = get_transform_hierarchy_children_begin(this); it != get_transform_hierarchy_children_end(this); ++it)
			{
				it->notify_global_change(void);
			}
			m_globalMatrixUptodate = false;
			m_moved = true;
		}

		bool is_global_matrix_uptodate(void) const
		{
			return m_globalMatrixUptodate;
		}

		bool is_local_matrix_uptodate(void) const
		{
			return m_localMatricesUptodate;
		}

		bool was_moved(void) const
		{
			return m_moved;
		}

		void set_moved(bool moved)
		{
			m_moved = moved;
		}

		inline void update_local_matrices(void)
		{
			if (!m_localMatricesUptodate)
			{
				m_localTranslationRotationMatrix = to_rotation4(m_localRotation) * to_translation4(m_localTranslation);
				m_localMatrix = to_scale4(m_localScale) * m_localTranslationRotationMatrix;
				m_localMatricesUptodate = true;
			}
		}

		inline void update_global_matrix(void)
		{
			if (!m_globalMatrixUptodate)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				const mat128 & local = get_local_matrix();
				m_globalMatrix = parent ? parent->get_global_matrix() * local : local;
				m_globalMatrixUptodate = true;
			}
		}

		inline void update_global_rotation_translation_matrix(void)
		{
			if (!m_globalTranslationRotationMatrixUptodate)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				m_globalTranslationRotationMatrix = parent ? parent->get_global_translation_rotation_matrix() * m_localTranslationRotationMatrix : m_localTranslationRotationMatrix;
				m_globalTranslationRotationMatrixUptodate = true;
			}
		}

		void extract_global_srt(vec128 * scaling, vec128 * rotation, vec128 * translation)
		{
			update_global_matrix();
			update_global_rotation_translation_matrix();

			// Extract translation
			vec128 t0 = vec128_permute<FUSE_W0, FUSE_W1, FUSE_W1, FUSE_W1>(m_globalTranslationRotationMatrix.c[0], m_globalTranslationRotationMatrix.c[1]);
			*translation = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_W1, FUSE_W1>(t0, m_globalTranslationRotationMatrix.c[2]);

			// Extract rotation
			mat128 r;
			r.c[0] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(m_globalTranslationRotationMatrix.c[0], vec128_zero());
			r.c[1] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(m_globalTranslationRotationMatrix.c[1], vec128_zero());
			r.c[2] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(m_globalTranslationRotationMatrix.c[2], vec128_zero());
			r.c[3] = m_globalTranslationRotationMatrix.c[3];
			*rotation = to_quat128(to_quaternion(reinterpret_cast<const float4x4&>(r)));

			// Extract scaling
			mat128 ss = m_globalMatrix * mat128_inverse4(r);
			vec128 s0 = vec128_select<FUSE_X0, FUSE_Y1, FUSE_Z0, FUSE_W0>(ss.c[0], ss.c[1]);
			*scaling  = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z1, FUSE_W1>(s0, ss.c[2]);

			// TODO: SSE mat128 to quat128 conversion, compute quat inverse instead of matrix inverse
		}

		mat128 m_globalMatrix;
		mat128 m_globalTranslationRotationMatrix;
		mat128 m_localMatrix;
		mat128 m_localTranslationRotationMatrix;

		vec128 m_localTranslation;
		vec128 m_localRotation;
		vec128 m_localScale;

		bool m_globalMatrixUptodate;
		bool m_localMatricesUptodate;
		bool m_globalTranslationRotationMatrixUptodate;
		bool m_moved;

	};

	

}