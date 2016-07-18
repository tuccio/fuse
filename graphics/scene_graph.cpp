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

scene_graph_node * scene_graph_node::find_root(void)
{
	scene_graph_node * current;
	for (current = this; current->get_parent() != nullptr; current = current->get_parent());
	return current;
}

bool scene_graph_node::is_ancestor(scene_graph_node * node) const
{
	return m_parent != nullptr && (m_parent == node || m_parent->is_ancestor(node));
}

bool scene_graph_node::is_descendant(scene_graph_node * node) const
{
	for (scene_graph_node * child : m_children)
	{
		if (child == node)
		{
			return true;
		}
		return child->is_descendant(node);
	}
	return false;
}

void scene_graph_node::set_parent(scene_graph_node * parent)
{
	mat128 oldMatrix = get_global_matrix();

	scene_graph_node * oldParent = m_parent;

	if (m_parent)
	{
		auto it = std::find(oldParent->m_children.begin(), oldParent->m_children.end(), this);
		if (it != oldParent->m_children.end())
		{
			oldParent->m_children.erase(it);
		}
	}

	switch_parent(parent);

	if (parent)
	{
		parent->m_children.push_back(this);
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
	m_camera.set_orientation(to_quaternion(get_global_rotation()));
	m_camera.set_position(to_float3(get_global_translation()));
}