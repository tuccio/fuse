//#include <fuse/scene_graph.hpp>
//#include <cassert>
//
//using namespace fuse;
//
//FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_node, 16)
//FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_group, 16)
//FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_geometry, 16)
//FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_camera, 16)
//
//scene_graph_node::~scene_graph_node(void)
//{
//
//	for (scene_graph_node_listener * listener : m_listeners)
//	{
//		listener->on_scene_graph_node_destruction(this);
//	}
//	
//	for (auto child : m_children)
//	{
//		child->on_parent_destruction();
//	}
//
//	if (m_parent)
//	{
//		m_parent->on_child_destruction(this);
//	}
//
//}
//
//void scene_graph_node::update(void)
//{
//	if (was_moved())
//	{
//		for (scene_graph_node_listener * listener : m_listeners)
//		{
//			listener->on_scene_graph_node_move(this);
//		}
//		set_moved(false);
//	}
//	for (scene_graph_node * n : m_children) n->update();
//}
//
//void scene_graph_node::on_parent_destruction(void)
//{
//	m_parent = nullptr;
//	delete this;
//}
//
//void scene_graph_node::on_child_destruction(scene_graph_node * child)
//{
//	auto it = std::find(m_children.begin(), m_children.end(), child);
//	assert(it != m_children.end() && "Consistency error: parent children list doesn't contain the child node.");
//	m_children.erase(it);
//}
//
//void scene_graph_node::clear_children(void)
//{
//	for (auto child : m_children)
//	{
//		delete child;
//	}
//	m_children.clear();
//}
//
//void scene_graph_camera::update(void)
//{
//
//	if (m_fixedCamera)
//	{
//		m_globalCamera = m_localCamera;
//	}
//	else if (was_moved())
//	{
//		XMMATRIX world = m_localCamera.get_world_matrix() * get_global_matrix();
//		m_globalCamera.set_world_matrix(world);
//	}
//
//	scene_graph_node::update();
//
//}
//
//void scene_graph_geometry::update(void)
//{
//
//	if (was_moved())
//	{
//		m_globalSphere = transform_affine(m_globalSphere, get_global_matrix());
//	}
//
//	scene_graph_node::update();
//}
