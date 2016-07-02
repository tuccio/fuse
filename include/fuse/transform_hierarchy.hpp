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
			m_globalTranslationRotationMatrix(false),
			m_localMatrixUptodate(false),
			m_moved(true) {}

	public:

		const XMMATRIX & get_global_matrix(void)
		{
			if (!m_globalMatrixUptodate)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				const XMMATRIX & local = get_local_matrix();
				m_globalMatrix = parent ? parent->get_global_matrix() * local : local;
				m_globalMatrixUptodate = true;
			}
			return m_globalMatrix;
		}

		const XMMATRIX & get_local_matrix(void)
		{
			if (!m_localMatrixUptodate)
			{
				m_localMatrix = XMMatrixScalingFromVector(m_localScale) * XMMatrixRotationQuaternion(m_localRotation) * XMMatrixTranslationFromVector(m_localTranslation);
				m_localMatrixUptodate = true;
			}
			return m_localMatrix;
		}

	protected:

		const XMMATRIX & get_global_translation_rotation_matrix(void)
		{
			if (!m_globalMatrixUptodate)
			{
				auto parent = get_transform_hierarchy_parent<T>(this);
				XMMATRIX trMatrix = XMQuaternionRotationMatrix(m_rotation) * XMMatrixTranslationFromVector(m_localTranslation);
				m_globalTranslationRotationMatrix = parent ? parent->get_global_matrix() * trMatrix : trMatrix;
				m_globalTranslationRotationMatrixUptodate = true;
			}
			return m_globalTranslationRotationMatrix;
		}

		void notify_local_change(void)
		{
			notify_global_change();
			m_localMatrixUptodate = false;
			m_moved = true;
		}

		void notify_global_change(void)
		{
			if (m_globalMatrixUptodate || m_globalTranslationRotationMatrix)
			{
				for (auto it = get_transform_hierarchy_children_begin(this); it != get_transform_hierarchy_children_end(this); ++it)
				{
					it->notify_global_change(void);
				}
			}
			m_globalTranslationRotationMatrix = false;
			m_globalMatrixUptodate = false;
			m_moved = true;
		}

		bool is_global_matrix_uptodate(void) const
		{
			return m_globalMatrixUptodate;
		}

		bool is_local_matrix_uptodate(void) const
		{
			return m_localMatrixUptodate;
		}

		bool was_moved(void) const
		{
			return m_moved;
		}

		void set_moved(bool moved)
		{
			m_moved = moved;
		}

	private:

		XMMATRIX m_globalMatrix;
		XMMATRIX m_globalTranslationRotationMatrix;
		XMMATRIX m_localMatrix;

		XMVECTOR m_localTranslation;
		XMVECTOR m_localRotation;
		XMVECTOR m_localScale;

		bool m_globalMatrixUptodate;
		bool m_globalTranslationRotationMatrixUptodate;
		bool m_localMatrixUptodate;
		bool m_moved;

	};

	

}