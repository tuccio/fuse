//#pragma once
//
//#include <fuse/properties_macros.hpp>
//#include <fuse/transform_hierarchy.hpp>
//#include <fuse/mesh.hpp>
//#include <fuse/gpu_mesh.hpp>
//#include <fuse/material.hpp>
//
//#include <fuse/camera.hpp>
//
//#include <memory>
//#include <vector>
//
//enum scene_graph_node_type
//{
//	FUSE_SCENE_GRAPH_GROUP,
//	FUSE_SCENE_GRAPH_GEOMETRY,
//	FUSE_SCENE_GRAPH_CAMERA
//};
//
//namespace fuse
//{
//
//	class alignas(16) scene_graph_node;
//
//	struct scene_graph_node_listener
//	{
//		virtual void on_scene_graph_node_destruction(scene_graph_node * node) = 0;
//		virtual void on_scene_graph_node_move(scene_graph_node * node) = 0;
//	};
//
//	class alignas(16) scene_graph_node :
//		public transform_hierarchy<scene_graph_node>
//	{
//
//	public:
//
//		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)
//
//		using children_iterator = std::vector<scene_graph_node*>::const_iterator;
//
//		virtual ~scene_graph_node(void);
//
//		virtual void update(void);
//
//		children_iterator begin(void)
//		{
//			return m_children.cbegin();
//		}
//
//		children_iterator end(void)
//		{
//			return m_children.cend();
//		}
//
//		inline bool is_root(void) const
//		{
//			return m_parent == nullptr;
//		}
//
//		inline bool is_leaf(void) const
//		{
//			return m_children.empty();
//		}
//
//		template <typename NodeType, typename ... Args>
//		inline NodeType * add_child(Args && ... args)
//		{
//			NodeType * child = new NodeType(this, std::forward<Args>(args) ...);
//			m_children.push_back(child);
//			return child;
//		}
//
//		void clear_children(void);
//
//		void add_listener(scene_graph_node_listener * listener)
//		{
//			m_listeners.push_back(listener);
//		}
//
//		void remove_listener(scene_graph_node_listener * listener)
//		{
//			auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
//			if (it != m_listeners.end())
//			{
//				m_listeners.erase(it);
//			}
//		}
//
//	protected:
//
//		scene_graph_node(scene_graph_node_type type, scene_graph_node * parent) :
//			m_type(type),
//			m_parent(parent) {}
//
//		void on_parent_destruction(void);
//		void on_child_destruction(scene_graph_node * child);
//
//	private:
//
//		scene_graph_node_type m_type;
//		scene_graph_node * m_parent;
//
//		std::vector<scene_graph_node*> m_children;
//		std::vector<scene_graph_node_listener*> m_listeners;
//
//	public:
//
//		FUSE_PROPERTIES_BY_VALUE (
//			(parent, m_parent)
//		)
//
//	};
//
//	template <>
//	inline scene_graph_node * get_transform_hierarchy_parent(transform_hierarchy<scene_graph_node> * node)
//	{
//		return static_cast<scene_graph_node*>(node)->get_parent();
//	}
//
//	template <>
//	inline typename scene_graph_node::children_iterator get_transform_hierarchy_children_begin(transform_hierarchy<scene_graph_node> * node)
//	{
//		return static_cast<scene_graph_node*>(node)->begin();
//	}
//
//	template <>
//	inline typename scene_graph_node::children_iterator get_transform_hierarchy_children_end(transform_hierarchy<scene_graph_node> * node)
//	{
//		return static_cast<scene_graph_node*>(node)->end();
//	}
//
//	/* Root node */
//
//	class alignas(16) scene_graph_group :
//		public scene_graph_node
//	{
//
//	public:
//
//		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)
//
//		scene_graph_group(void) :
//			scene_graph_node(FUSE_SCENE_GRAPH_GROUP, nullptr) {}
//
//	};
//
//	/* Camera node */
//
//	class alignas(16) scene_graph_camera :
//		public scene_graph_node
//	{
//
//	public:
//
//		scene_graph_camera(void) :
//			scene_graph_node(FUSE_SCENE_GRAPH_CAMERA, nullptr),
//			m_fixedCamera(false) {}
//
//		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)
//
//		const camera * get_global_camera(void)
//		{
//			return &m_globalCamera;
//		}
//
//		camera * get_local_camera(void)
//		{
//			return &m_localCamera;
//		}
//
//		void update(void) override;
//
//	private:
//
//		camera m_localCamera;
//		camera m_globalCamera;
//
//		bool m_fixedCamera;
//
//		FUSE_PROPERTIES_BY_VALUE(
//			(fixed_camera, m_fixedCamera)
//		)
//
//	};
//
//	/* Geometry node */
//
//	class alignas(16) scene_graph_geometry :
//		public scene_graph_node
//	{
//
//	public:
//
//		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)
//
//		void update(void) override;
//
//	private:
//
//		scene_graph_geometry(void) :
//			scene_graph_node(FUSE_SCENE_GRAPH_GROUP, nullptr) {}
//
//		friend class scene_graph;
//		friend class scene_graph_node;
//
//		sphere       m_localSphere;
//		sphere       m_globalSphere;
//
//		material_ptr m_material;
//		mesh_ptr     m_mesh;
//		gpu_mesh_ptr m_gpumesh;
//
//	public:
//
//		FUSE_PROPERTIES_BY_CONST_REFERENCE(
//			(local_bounding_sphere, m_localSphere)
//			(material, m_material)
//			(mesh, m_mesh)
//			(gpu_mesh, m_gpumesh)
//		)
//
//		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
//			(global_bounding_sphere, m_globalSphere)
//		)
//
//	};
//
//	/* Scene graph */
//
//	class scene_graph
//	{
//
//	public:
//
//		scene_graph(void) :
//			m_root(new scene_graph_group) {}
//
//		scene_graph(const scene_graph &) = delete;
//		scene_graph(scene_graph &&) = default;
//
//		void update(void)
//		{
//			m_root->update();
//		}
//
//		void clear(void)
//		{
//			m_root->clear_children();
//		}
//
//		scene_graph_node * get_root(void) const
//		{
//			m_root.get();
//		}
//
//		inline scene_graph_node * operator-> (void) const
//		{
//			return m_root.get();
//		}
//
//	private:
//
//		std::unique_ptr<scene_graph_group> m_root;
//
//	};
//
//}