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
	void set_transform_hierarchy_parent(transform_hierarchy<TransformHierarchyNode> * node, transform_hierarchy<TransformHierarchyNode> * parent);

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
			m_globalSRTUptodate(false),
			m_localRotation(quat128_identity()),
			m_localScale(vec128_one()),
			m_localTranslation(vec128_zero()),
			m_localMatrix(mat128_identity()),
			m_globalMatrix(mat128_identity()),
			m_globalScale(vec128_one()),
			m_globalRotation(quat128_identity()),
			m_globalTranslation(vec128_zero()),
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

		const mat128 & get_local_translation_rotation_matrix(void)
		{
			update_local_matrices();
			return m_localTranslationRotationMatrix;
		}

		inline void set_local_translation(const float3 & t)
		{
			set_local_translation(to_vec128(t));
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
			notify_local_change();
		}

		inline void FUSE_VECTOR_CALL set_local_rotation(vec128 r)
		{
			m_localRotation = r;
			notify_local_change();
		}

		inline void FUSE_VECTOR_CALL set_local_scale(vec128 s)
		{
			m_localScale = s;
			notify_local_change();
		}

		inline vec128 get_local_translation(void) const
		{
			return m_localTranslation;
		}

		inline vec128 get_local_rotation(void) const
		{
			return m_localRotation;
		}

		inline vec128 get_local_scale(void) const
		{
			return m_localScale;
		}

		inline vec128 get_global_translation(void)
		{
			update_global_srt();
			return m_globalTranslation;
		}

		inline vec128 get_global_rotation(void)
		{
			update_global_srt();
			return m_globalRotation;
		}

		inline vec128 get_global_scale(void)
		{
			update_global_srt();
			return m_globalScale;
		}

	protected:

		const mat128 & get_global_translation_rotation_matrix(void)
		{
			update_global_rotation_translation_matrix();
			return m_globalTranslationRotationMatrix;
		}

		void notify_local_change(void)
		{
			m_localMatricesUptodate = false;
			m_moved                 = false;
			notify_global_change();
		}

		void notify_global_change(void)
		{
			if (!m_moved)
			{
				m_moved = true;
				m_globalMatrixUptodate                    = false;
				m_globalTranslationRotationMatrixUptodate = false;
				m_globalSRTUptodate                       = false;
				for (auto it = get_transform_hierarchy_children_begin<T>(this); it != get_transform_hierarchy_children_end<T>(this); ++it)
				{
					(*it)->notify_global_change();
				}
			}
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

		inline bool update_local_matrices(void)
		{
			bool update = !m_localMatricesUptodate;
			if (update)
			{
				m_localTranslationRotationMatrix = to_rotation4(m_localRotation) * to_translation4(m_localTranslation);
				m_localMatrix = to_scale4(m_localScale) * m_localTranslationRotationMatrix;
				m_localMatricesUptodate = true;
			}
			return update;
		}

		inline bool update_global_matrix(void)
		{
			bool update = !m_globalMatrixUptodate;
			if (update)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				const mat128 & local = get_local_matrix();
				m_globalMatrix = parent ? local * parent->get_global_matrix() : local;
				m_globalMatrixUptodate = true;
			}
			return update;
		}

		inline bool update_global_rotation_translation_matrix(void)
		{
			bool update = !m_globalTranslationRotationMatrixUptodate;
			if (update)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				const mat128 & local = get_local_translation_rotation_matrix();
				m_globalTranslationRotationMatrix = parent ? local * parent->get_global_translation_rotation_matrix() : local;
				m_globalTranslationRotationMatrixUptodate = true;
			}
			return update;
		}

		static inline void extract_srt(const mat128 & SRT, const mat128 & RT, vec128 * scale, vec128 * rotation, vec128 * translation)
		{
			// Extract translation
			vec128 t0 = vec128_permute<FUSE_W0, FUSE_W1, FUSE_W1, FUSE_W1>(RT.c[0], RT.c[1]);
			*translation = vec128_permute<FUSE_X0, FUSE_Y0, FUSE_W1, FUSE_W1>(t0, RT.c[2]);

			// Extract rotation
			mat128 r;
			r.c[0] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(RT.c[0], vec128_zero());
			r.c[1] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(RT.c[1], vec128_zero());
			r.c[2] = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z0, FUSE_W1>(RT.c[2], vec128_zero());
			r.c[3] = RT.c[3];
			*rotation = to_quat128(r);

			// Extract scaling
			mat128 ss = SRT * mat128_inverse4(RT); // TODO: compute quat inverse instead of matrix inverse
			vec128 s0 = vec128_select<FUSE_X0, FUSE_Y1, FUSE_Z0, FUSE_W0>(ss.c[0], ss.c[1]);
			*scale = vec128_select<FUSE_X0, FUSE_Y0, FUSE_Z1, FUSE_W1>(s0, ss.c[2]);
		}

		inline bool update_global_srt(void)
		{
			bool update = !m_globalSRTUptodate;
			if (update)
			{
				extract_srt(get_global_matrix(), get_global_translation_rotation_matrix(), &m_globalScale, &m_globalRotation, &m_globalTranslation);
				m_globalSRTUptodate = true;
			}
			return update;
		}

		inline void switch_parent(transform_hierarchy * parent)
		{
			mat128 invParentSRT = parent ? mat128_inverse4(parent->get_global_matrix()) : mat128_identity();
			mat128 invParentRT  = parent ? mat128_inverse4(parent->get_global_translation_rotation_matrix()) : mat128_identity();

			mat128 newSRT = get_global_matrix() * invParentSRT;
			mat128 newRT  = get_global_translation_rotation_matrix() * invParentRT;
			
			extract_srt(newSRT, newRT, &m_localScale, &m_localRotation, &m_localTranslation);
			set_transform_hierarchy_parent<T>(this, parent);
			notify_local_change();
		}

		mat128 m_globalMatrix;
		mat128 m_globalTranslationRotationMatrix;
		mat128 m_localMatrix;
		mat128 m_localTranslationRotationMatrix;

		vec128 m_localTranslation;
		vec128 m_localRotation;
		vec128 m_localScale;

		vec128 m_globalTranslation;
		vec128 m_globalRotation;
		vec128 m_globalScale;

		bool m_globalMatrixUptodate;
		bool m_localMatricesUptodate;
		bool m_globalTranslationRotationMatrixUptodate;
		bool m_globalSRTUptodate;
		bool m_moved;

	};

	

}