#include "visual_debugger.hpp"

#include <fuse/render_resource_manager.hpp>

using namespace fuse;

visual_debugger::visual_debugger(void) :
	m_renderer(nullptr) {}

bool visual_debugger::init(ID3D12Device * device, const debug_renderer_configuration & cfg)
{
	m_drawBoundingVolumes            = false;
	m_drawOctree                     = false;
	m_drawSkydome                    = false;
	m_drawSelectedNodeBoundingVolume = true;
	m_texturesScale                  = .25f;
	return debug_renderer::init(device, cfg);
}

void visual_debugger::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{
	draw_persistent_objects();
	debug_renderer::render(device, commandQueue, commandList, ringBuffer, cbPerFrame, renderTarget, depthBuffer);
}

void visual_debugger::add_persistent(const char_t * name, const aabb & boundingBox, const color_rgba & color)
{
	m_persistentObjects.emplace(name, draw_info{ boundingBox, color });
}

void visual_debugger::add_persistent(const char_t * name, const frustum & f, const color_rgba & color)
{
	m_persistentObjects.emplace(name, draw_info{ f, color });
}

void visual_debugger::add_persistent(const char_t * name, const ray & r, const color_rgba & color)
{
	auto it = m_persistentObjects.emplace(name, draw_info{ r, color });
	if (!it.second)
	{
		(it.first->second) = draw_info{ r, color };
	}
}

void visual_debugger::remove_persistent(const char_t * name)
{
	auto it = m_persistentObjects.find(name);

	if (it != m_persistentObjects.end())
	{
		m_persistentObjects.erase(it);
	}
}

class persistent_object_visitor :
	public boost::static_visitor<void>
{

public:

	persistent_object_visitor(debug_renderer * r, const color_rgba & c) :
		m_renderer(r), m_color(c) {}

	void operator()(const aabb & boundingBox) const
	{
		m_renderer->add(boundingBox, m_color);
	}

	void operator()(const frustum & f) const
	{
		m_renderer->add(f, m_color);
	}

	void operator()(const ray & r) const
	{
		m_renderer->add(r, m_color);
	}

private:

	debug_renderer * m_renderer;
	color_rgba m_color;

};


void visual_debugger::draw_persistent_objects(void)
{
	for (auto & p : m_persistentObjects)
	{
		boost::apply_visitor(persistent_object_visitor{ this, p.second.color }, p.second.geometry);
	}
}