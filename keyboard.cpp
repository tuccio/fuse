#include <fuse/keyboard.hpp>

using namespace fuse;

enum keyboard_button_status
{
	FUSE_KEYBOARD_STATUS_BUTTON_DOWN = 1
};

keyboard::keyboard(void)
{
	memset(m_status, 0, sizeof(m_status));
}

bool keyboard::is_button_down(keyboard_vk key) const
{
	return (m_status[key] & FUSE_KEYBOARD_STATUS_BUTTON_DOWN) != 0;
}

bool keyboard::is_button_up(keyboard_vk key) const
{
	return (m_status[key] & FUSE_KEYBOARD_STATUS_BUTTON_DOWN) == 0;
}

void keyboard::post_keyboard_button_down(keyboard_vk key)
{

	m_status[key] &= FUSE_KEYBOARD_STATUS_BUTTON_DOWN;

	keyboard_event_info event = { FUSE_KEYBOARD_EVENT_BUTTON_DOWN, key, false };
	post_keyboard_event(event);

}

void keyboard::post_keyboard_button_up(keyboard_vk key)
{

	m_status[key] &= ~FUSE_KEYBOARD_STATUS_BUTTON_DOWN;

	keyboard_event_info event = { FUSE_KEYBOARD_EVENT_BUTTON_UP, key, false };
	post_keyboard_event(event);

}

void keyboard::post_keyboard_event(const keyboard_event_info & event)
{

	notify<const keyboard &, const keyboard_event_info &>(
		&keyboard_listener::on_keyboard_event, *this, event);

}