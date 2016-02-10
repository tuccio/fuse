#include "visual_debugger.hpp"

using namespace fuse;

visual_debugger::visual_debugger(void) :
	m_renderer(nullptr) {}

bool visual_debugger::init(debug_renderer * renderer)
{
	m_renderer = renderer;
	m_drawBoundingVolumes = false;
	return true;
}

void visual_debugger::shutdown(void)
{
	m_renderer = nullptr;
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
	m_renderer->render(device, commandQueue, commandList, ringBuffer, cbPerFrame, renderTarget, depthBuffer);
}

void visual_debugger::add_persistent(const char * name, const aabb & boundingBox, const color_rgba & color)
{
	m_persistentObjects.emplace(name, draw_info{ boundingBox, color });
}

void visual_debugger::add_persistent(const char * name, const frustum & f, const color_rgba & color)
{
	m_persistentObjects.emplace(name, draw_info{ f, color });
}

void visual_debugger::remove_presistent(const char * name)
{

	auto it = m_persistentObjects.find(name);

	if (it != m_persistentObjects.end())
	{
		m_persistentObjects.erase(it);
	}

}

void visual_debugger::add(const frustum & f, const color_rgba & color)
{
	m_renderer->add(f, color);
}

void visual_debugger::add(const aabb & boundingBox, const color_rgba & color)
{
	m_renderer->add(boundingBox, color);
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

private:

	debug_renderer * m_renderer;
	color_rgba m_color;

};


void visual_debugger::draw_persistent_objects(void)
{

	for (auto & p : m_persistentObjects)
	{
		boost::apply_visitor(persistent_object_visitor{ m_renderer, p.second.color }, p.second.geometry);
	}

}