#pragma once

#include <fuse/core.hpp>
#include <fuse/transform_hierarchy.hpp>
#include <fuse/mesh.hpp>
#include <fuse/gpu_mesh.hpp>
#include <fuse/material.hpp>

#include <fuse/camera.hpp>

#include <memory>
#include <vector>

enum scene_graph_node_type
{
	FUSE_SCENE_GRAPH_GROUP,
	FUSE_SCENE_GRAPH_GEOMETRY,
	FUSE_SCENE_GRAPH_CAMERA
};

namespace fuse
{

	class alignas(16) scene_graph_node;

	struct scene_graph_node_listener
	{
		virtual void on_scene_graph_node_destruction(scene_graph_node * node) {}
		virtual void on_scene_graph_node_move(scene_graph_node * node, const mat128 & oldTransform, const mat128 & newTransform) {}
	};

	class alignas(16) scene_graph_node :
		public transform_hierarchy<scene_graph_node>
	{

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		using children_iterator       = std::vector<scene_graph_node*>::iterator;
		using children_const_iterator = std::vector<scene_graph_node*>::const_iterator;

		virtual ~scene_graph_node(void);

		children_iterator begin(void)
		{
			return m_children.begin();
		}

		children_iterator end(void)
		{
			return m_children.end();
		}

		children_const_iterator begin(void) const
		{
			return m_children.cbegin();
		}

		children_const_iterator end(void) const
		{
			return m_children.cend();
		}

		inline bool is_root(void) const
		{
			return m_parent == nullptr;
		}

		inline bool is_leaf(void) const
		{
			return m_children.empty();
		}

		template <typename NodeType, typename ... Args>
		inline NodeType * add_child(Args && ... args)
		{
			NodeType * child = new NodeType(this, std::forward<Args>(args) ...);
			m_children.push_back(child);
			return child;
		}

		void clear_children(void);

		void add_listener(scene_graph_node_listener * listener)
		{
			m_listeners.push_back(listener);
		}

		void remove_listener(scene_graph_node_listener * listener)
		{
			auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
			if (it != m_listeners.end())
			{
				m_listeners.erase(it);
			}
		}

		void update(void);

		scene_graph_node * find_root(void);

		void set_parent(scene_graph_node * parent);

		inline void set_parent_raw(scene_graph_node * parent)
		{
			m_parent = parent;
		}

		bool is_ancestor(scene_graph_node * node) const;
		bool is_descendant(scene_graph_node * node) const;

	protected:

		scene_graph_node(scene_graph_node_type type, scene_graph_node * parent) :
			m_type(type),
			m_parent(parent) {}

		void on_parent_destruction(void);
		void on_child_destruction(scene_graph_node * child);

		virtual void update_impl(void) {}

	private:

		scene_graph_node_type m_type;
		scene_graph_node * m_parent;

		std::vector<scene_graph_node*> m_children;
		std::vector<scene_graph_node_listener*> m_listeners;

		string_t m_name;

		inline transform_hierarchy<scene_graph_node> * get_transform_hierarchy_parent(void)
		{
			return m_parent;
		}

		inline void set_transform_hierarchy_parent(transform_hierarchy<scene_graph_node> * parent)
		{
			m_parent = static_cast<scene_graph_node*>(parent);
		}

		inline children_iterator get_transform_hierarchy_children_begin(void)
		{
			return m_children.begin();
		}

		inline children_iterator get_transform_hierarchy_children_end(void)
		{
			return m_children.end();
		}

		inline children_const_iterator get_transform_hierarchy_children_begin(void) const
		{
			return m_children.cbegin();
		}

		inline children_const_iterator get_transform_hierarchy_children_end(void) const
		{
			return m_children.cend();
		}

	public:

		FUSE_PROPERTIES_STRING(
			(name, m_name)
		)

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
			(parent, m_parent)
			(type, m_type)
		)
	};

	template <>
	inline scene_graph_node * get_transform_hierarchy_parent(transform_hierarchy<scene_graph_node> * node)
	{
		return static_cast<scene_graph_node*>(node)->get_parent();
	}

	template <>
	inline void set_transform_hierarchy_parent(transform_hierarchy<scene_graph_node> * node, transform_hierarchy<scene_graph_node> * parent)
	{
		static_cast<scene_graph_node*>(node)->set_parent_raw(static_cast<scene_graph_node*>(parent));
	}

	template <>
	inline typename scene_graph_node::children_iterator get_transform_hierarchy_children_begin(transform_hierarchy<scene_graph_node> * node)
	{
		return static_cast<scene_graph_node*>(node)->begin();
	}

	template <>
	inline typename scene_graph_node::children_iterator get_transform_hierarchy_children_end(transform_hierarchy<scene_graph_node> * node)
	{
		return static_cast<scene_graph_node*>(node)->end();
	}

	/* Group node */

	class alignas(16) scene_graph_group :
		public scene_graph_node
	{

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		scene_graph_group(void) :
			scene_graph_group(nullptr) {}

		scene_graph_group(scene_graph_node * parent) :
			scene_graph_node(FUSE_SCENE_GRAPH_GROUP, parent) {}

	};

	/* Camera node */

	class alignas(16) scene_graph_camera :
		public scene_graph_node
	{

	public:

		scene_graph_camera(void) :
			scene_graph_camera(nullptr) {}

		scene_graph_camera(scene_graph_node * parent) :
			scene_graph_node(FUSE_SCENE_GRAPH_CAMERA, parent) {}

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		camera * get_camera(void)
		{
			return &m_camera;
		}

		const camera * get_camera(void) const
		{
			return &m_camera;
		}

	protected:

		void update_impl(void) override;

	private:

		camera m_camera;

	};

	/* Geometry node */

	class alignas(16) scene_graph_geometry :
		public scene_graph_node
	{

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		sphere get_global_bounding_sphere(void)
		{
			return transform_affine(m_localSphere, get_global_matrix());
		}

	private:

		scene_graph_geometry(void) :
			scene_graph_geometry(nullptr) {}

		scene_graph_geometry(scene_graph_node * parent) :
			scene_graph_node(FUSE_SCENE_GRAPH_GEOMETRY, parent) {}

		friend class scene_graph;
		friend class scene_graph_node;

		sphere       m_localSphere;

		material_ptr m_material;
		mesh_ptr     m_mesh;
		gpu_mesh_ptr m_gpumesh;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(local_bounding_sphere, m_localSphere)
			(material, m_material)
			(mesh, m_mesh)
			(gpu_mesh, m_gpumesh)
		)

	};

	/* Scene graph */

	class scene_graph
	{

	public:

		scene_graph(void) :
			m_root(new scene_graph_group()) {}

		scene_graph(const scene_graph &) = delete;
		scene_graph(scene_graph &&) = default;

		void update(void)
		{
			m_root->update();
		}

		void clear(void)
		{
			m_root = std::make_unique<scene_graph_group>();
		}

		scene_graph_node * get_root(void) const
		{
			return m_root.get();
		}

		inline scene_graph_node * operator-> (void) const
		{
			return m_root.get();
		}

		template <typename Visitor>
		void visit(Visitor v)
		{
			visit_impl(m_root.get(), v);
		}

	private:

		template <typename Visitor>
		void visit_impl(scene_graph_node * n, Visitor v)
		{
			v(n);

			for (scene_graph_node * c : *n)
			{
				visit_impl(c, v);
			}
		}

		std::unique_ptr<scene_graph_group> m_root;

	};

}