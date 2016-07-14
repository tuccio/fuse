#pragma once

#include <fuse/core.hpp>
#include <fuse/math.hpp>
#include <fuse/priority_observers.hpp>

#include <Windows.h>

enum mouse_vk
{
	FUSE_MOUSE_VK_UNKNOWN = 0,
	FUSE_MOUSE_VK_MOUSE1,
	FUSE_MOUSE_VK_MOUSE2,
	FUSE_MOUSE_VK_MOUSE3,
	FUSE_MOUSE_VK_MOUSE4,
	FUSE_MOUSE_VK_MOUSE5,
	FUSE_MOUSE_VK_MOUSE6,
	FUSE_MOUSE_VK_MOUSE7,
	FUSE_MOUSE_VK_MOUSE8,
	FUSE_MOUSE_VK_MOUSE9,
	FUSE_MOUSE_VK_MOUSE10,
	FUSE_MOUSE_VK_MOUSE11,
	FUSE_MOUSE_VK_MOUSE12,
	FUSE_MOUSE_VK_MOUSE13,
	FUSE_MOUSE_VK_MOUSE14,
	FUSE_MOUSE_VK_MOUSE15,
	FUSE_MOUSE_VK_MOUSE16,
	FUSE_MOUSE_VK_MAX
};

enum mouse_event_type
{
	FUSE_MOUSE_EVENT_MOVE,
	FUSE_MOUSE_EVENT_BUTTON_DOWN,
	FUSE_MOUSE_EVENT_BUTTON_UP,
	FUSE_MOUSE_EVENT_WHEEL,
};

namespace fuse
{

	class mouse;

	struct mouse_event_info
	{

		mouse_event_type type;
		int2             position;

		union
		{
			mouse_vk key;
			int      wheel;
		};

	};

	struct mouse_listener
	{
		virtual bool on_mouse_event(const mouse & mouse, const mouse_event_info & event) = 0;
	};

	typedef bool (*mouse_callback) (const mouse & mouse, const mouse_event_info & event);

	class mouse :
		public priority_observable<mouse_callback, mouse_listener>
	{

	public:

		mouse(void);

		bool is_button_down(mouse_vk key) const;
		bool is_button_up(mouse_vk key) const;

		void post_mouse_event(const mouse_event_info & event);

		void post_mouse_button_down(mouse_vk key);
		void post_mouse_button_up(mouse_vk key);
		void post_mouse_move(const int2 & position);
		void post_mouse_wheel(int wheel);

		inline const int2 & get_last_click_position(mouse_vk key)
		{
			return m_lastClickPosition[key];
		}

	private:

		int m_status[FUSE_MOUSE_VK_MAX];

		int2 m_lastClickPosition[FUSE_MOUSE_VK_MAX];

		int2 m_position;

		HWND m_autoCenterWindow;
		bool m_autoCenter;
		bool m_centering;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(position, m_position)
		)

		FUSE_PROPERTIES_BY_VALUE(
			(auto_center, m_autoCenter)
			(auto_center_window, m_autoCenterWindow)
		)

	};

}