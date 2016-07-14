#pragma once

#include <Windows.h>

#include <fuse/priority_observers.hpp>

enum keyboard_vk
{
	FUSE_KEYBOARD_VK_UNKNOWN,
	FUSE_KEYBOARD_VK_0 = '0',
	FUSE_KEYBOARD_VK_1,
	FUSE_KEYBOARD_VK_2,
	FUSE_KEYBOARD_VK_3,
	FUSE_KEYBOARD_VK_4,
	FUSE_KEYBOARD_VK_5,
	FUSE_KEYBOARD_VK_6,
	FUSE_KEYBOARD_VK_7,
	FUSE_KEYBOARD_VK_8,
	FUSE_KEYBOARD_VK_9,
	FUSE_KEYBOARD_VK_A = 'A',
	FUSE_KEYBOARD_VK_B,
	FUSE_KEYBOARD_VK_C,
	FUSE_KEYBOARD_VK_D,
	FUSE_KEYBOARD_VK_E,
	FUSE_KEYBOARD_VK_F,
	FUSE_KEYBOARD_VK_G,
	FUSE_KEYBOARD_VK_H,
	FUSE_KEYBOARD_VK_I,
	FUSE_KEYBOARD_VK_J,
	FUSE_KEYBOARD_VK_K,
	FUSE_KEYBOARD_VK_L,
	FUSE_KEYBOARD_VK_M,
	FUSE_KEYBOARD_VK_N,
	FUSE_KEYBOARD_VK_O,
	FUSE_KEYBOARD_VK_P,
	FUSE_KEYBOARD_VK_Q,
	FUSE_KEYBOARD_VK_R,
	FUSE_KEYBOARD_VK_S,
	FUSE_KEYBOARD_VK_T,
	FUSE_KEYBOARD_VK_U,
	FUSE_KEYBOARD_VK_V,
	FUSE_KEYBOARD_VK_W,
	FUSE_KEYBOARD_VK_X,
	FUSE_KEYBOARD_VK_Y,
	FUSE_KEYBOARD_VK_Z,
	FUSE_KEYBOARD_VK_LEFT_ARROW = 256,
	FUSE_KEYBOARD_VK_RIGHT_ARROW,
	FUSE_KEYBOARD_VK_DOWN_ARROW,
	FUSE_KEYBOARD_VK_UP_ARROW,
	FUSE_KEYBOARD_VK_F1,
	FUSE_KEYBOARD_VK_F2,
	FUSE_KEYBOARD_VK_F3,
	FUSE_KEYBOARD_VK_F4,
	FUSE_KEYBOARD_VK_F5,
	FUSE_KEYBOARD_VK_F6,
	FUSE_KEYBOARD_VK_F7,
	FUSE_KEYBOARD_VK_F8,
	FUSE_KEYBOARD_VK_F9,
	FUSE_KEYBOARD_VK_F10,
	FUSE_KEYBOARD_VK_F11,
	FUSE_KEYBOARD_VK_F12,
	FUSE_KEYBOARD_VK_F13,
	FUSE_KEYBOARD_VK_F14,
	FUSE_KEYBOARD_VK_F15,
	FUSE_KEYBOARD_VK_F16,
	FUSE_KEYBOARD_VK_F17,
	FUSE_KEYBOARD_VK_F18,
	FUSE_KEYBOARD_VK_F19,
	FUSE_KEYBOARD_VK_F20,
	FUSE_KEYBOARD_VK_F21,
	FUSE_KEYBOARD_VK_F22,
	FUSE_KEYBOARD_VK_F23,
	FUSE_KEYBOARD_VK_F24,
	FUSE_KEYBOARD_VK_ENTER,
	FUSE_KEYBOARD_VK_MAX

};

enum keyboard_event_type
{
	FUSE_KEYBOARD_EVENT_BUTTON_DOWN,
	FUSE_KEYBOARD_EVENT_BUTTON_UP
};

namespace fuse
{

	class keyboard;

	struct keyboard_event_info
	{
		keyboard_event_type type;
		keyboard_vk key;
		bool repeating;
	};
	
	struct keyboard_listener
	{
		virtual bool on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event) = 0;
	};

	typedef bool (*keyboard_callback) (const keyboard & keyboard, const keyboard_event_info & event);

	class keyboard :
		public priority_observable<keyboard_callback, keyboard_listener>
	{

	public:

		keyboard(void);

		bool is_button_down(keyboard_vk key) const;
		bool is_button_up(keyboard_vk key) const;

		void post_keyboard_event(const keyboard_event_info & event);

		void post_keyboard_button_down(keyboard_vk key);
		void post_keyboard_button_up(keyboard_vk key);

	private:

		int m_status[FUSE_KEYBOARD_VK_MAX];

	};

	inline keyboard_vk keyboard_vk_from_win32(int vk)
	{
		keyboard_vk key = FUSE_KEYBOARD_VK_UNKNOWN;

		if ((vk >= 'A' && vk <= 'Z') ||
			(vk >= '0' && vk <= '9'))
		{
			key = static_cast<keyboard_vk>(vk);
		}
		else if (vk >= VK_F1 && vk <= VK_F24)
		{
			key = static_cast<keyboard_vk>(vk - VK_F1 + FUSE_KEYBOARD_VK_F1);
		}
		else
		{
			switch (vk)
			{

			case VK_LEFT:
				key = FUSE_KEYBOARD_VK_LEFT_ARROW;
				break;

			case VK_RIGHT:
				key = FUSE_KEYBOARD_VK_RIGHT_ARROW;
				break;

			case VK_UP:
				key = FUSE_KEYBOARD_VK_UP_ARROW;
				break;

			case VK_DOWN:
				key = FUSE_KEYBOARD_VK_DOWN_ARROW;
				break;

			case VK_RETURN:
				key = FUSE_KEYBOARD_VK_ENTER;

			}
		}

		return key;
	}

}