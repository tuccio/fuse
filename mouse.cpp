#include <fuse/mouse.hpp>
#include <fuse/core.hpp>

using namespace fuse;

mouse::mouse(void) :
	m_autoCenterWindow(NULL),
	m_autoCenter(false),
	m_centering(false) {}

enum mouse_button_status
{
	FUSE_MOUSE_STATUS_BUTTON_DOWN = 1
};

bool mouse::is_button_down(mouse_vk key) const
{
	return (m_status[key] & FUSE_MOUSE_STATUS_BUTTON_DOWN) != 0;
}

bool mouse::is_button_up(mouse_vk key) const
{
	return (m_status[key] & FUSE_MOUSE_STATUS_BUTTON_DOWN) != 0;
}

void mouse::post_mouse_button_down(mouse_vk key)
{

	m_status[key] &= FUSE_MOUSE_STATUS_BUTTON_DOWN;

	mouse_event_info event = { FUSE_MOUSE_EVENT_BUTTON_DOWN, m_position };
	event.key = key;

	post_mouse_event(event);

}

void mouse::post_mouse_button_up(mouse_vk key)
{

	m_status[key] &= ~FUSE_MOUSE_STATUS_BUTTON_DOWN;

	mouse_event_info event = { FUSE_MOUSE_EVENT_BUTTON_UP, m_position };
	event.key = key;
	
	post_mouse_event(event);

}

void mouse::post_mouse_move(const XMINT2 & position)
{
	m_position = position;
	mouse_event_info event = { FUSE_MOUSE_EVENT_MOVE, m_position };
	post_mouse_event(event);
}

void mouse::post_mouse_wheel(int wheel)
{
	mouse_event_info event = { FUSE_MOUSE_EVENT_WHEEL, m_position };
	event.wheel = wheel;
	post_mouse_event(event);
}


void mouse::post_mouse_event(const mouse_event_info & event)
{

	notify<const mouse &, const mouse_event_info &>(
		&mouse_listener::on_mouse_event, *this, event);

}