#include <fuse/scene_graph.hpp>
#include <cassert>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_node, 16)
FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_group, 16)
FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_geometry, 16)
FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene_graph_camera, 16)

scene_graph_node::~scene_graph_node(void)
{
	for (scene_graph_node_listener * listener : m_listeners)
	{
		listener->on_scene_graph_node_destruction(this);
	}
	
	for (auto child : m_children)
	{
		child->on_parent_destruction();
	}

	if (m_parent)
	{
		m_parent->on_child_destruction(this);
	}
}

void scene_graph_node::update(void)
{
	if (was_moved())
	{
		mat128 oldTransform = m_globalMatrix;
		mat128 newTransform = get_global_matrix();

		update_impl();

		for (scene_graph_node_listener * listener : m_listeners)
		{
			listener->on_scene_graph_node_move(this, oldTransform, newTransform);
		}

		set_moved(false);
	}

	for (scene_graph_node * n : m_children) n->update();
}

void scene_graph_node::on_parent_destruction(void)
{
	m_parent = nullptr;
	delete this;
}

void scene_graph_node::on_child_destruction(scene_graph_node * child)
{
	auto it = std::find(m_children.begin(), m_children.end(), child);
	assert(it != m_children.end() && "Consistency error: parent children list doesn't contain the child node.");
	m_children.erase(it);
}

void scene_graph_node::clear_children(void)
{
	for (auto child : m_children)
	{
		delete child;
	}
	m_children.clear();
}

void scene_graph_camera::update_impl(void)
{
	vec128 s, r, t;
	extract_global_srt(&s, &r, &t);
	m_camera.set_orientation(to_quaternion(r));
	m_camera.set_position(to_float3(t));
}