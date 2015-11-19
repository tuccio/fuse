#pragma once

#include <Rocket/Core.h>

#include <Windows.h>

namespace fuse
{

	int  rocked_key_modifier(void);
	bool rocket_keyboard_input_translate(Rocket::Core::Context * context, WPARAM wParam, LPARAM lParam);

}