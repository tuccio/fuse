#include <fuse/rocket_input.hpp>
#include <fuse/logger.hpp>
#include <cctype>

#define KEY_DOWN_LPARAM(Flags) (((KF_UP << 16) & Flags) == 0)
#define KEY_DOWN(Flags) (0x8000 & Flags)

static bool g_capsLock;;

int fuse::rocked_key_modifier(void)
{

	int modifier = 0;

	if (KEY_DOWN(GetAsyncKeyState(VK_SHIFT)))
	{
		modifier |= Rocket::Core::Input::KM_SHIFT;
	}

	if (KEY_DOWN(GetAsyncKeyState(VK_CONTROL)))
	{
		modifier |= Rocket::Core::Input::KM_CTRL;
	}

	if (KEY_DOWN(GetAsyncKeyState(VK_MENU)))
	{
		modifier |= Rocket::Core::Input::KM_ALT;
	}

	if (GetAsyncKeyState(VK_CAPITAL) & 0x8001)
	{
		g_capsLock = !g_capsLock;
	}

	if (g_capsLock)
	{
		modifier |= Rocket::Core::Input::KM_CAPSLOCK;
	}

	if (KEY_DOWN(GetAsyncKeyState(VK_NUMLOCK)))
	{
		modifier |= Rocket::Core::Input::KM_NUMLOCK;
	}

	return modifier;

}

static Rocket::Core::Input::KeyIdentifier rocket_keyboard_map_key(WPARAM wParam)
{

	using namespace Rocket::Core::Input;

	KeyIdentifier key = KI_UNKNOWN;

	if (wParam >= 'A' && wParam <= 'Z')
	{
		key = static_cast<KeyIdentifier>(KI_A + (wParam - 'A'));
	}
	else if (wParam >= '0' && wParam <= '9')
	{
		key = static_cast<KeyIdentifier>(KI_0 + (wParam - '0'));
	}
	else if (wParam >= VK_F1 && wParam <= VK_F12)
	{
		key = static_cast<KeyIdentifier>(KI_F1 + (wParam - VK_F1));
	}
	else
	{

		switch (wParam)
		{
		case VK_LEFT:
			key = KI_LEFT;
			break;
		case VK_RIGHT:
			key = KI_RIGHT;
			break;
		case VK_UP:
			key = KI_UP;
			break;
		case VK_DOWN:
			key = KI_DOWN;
			break;
		case VK_BACK:
			key = KI_BACK;
			break;
		case VK_RETURN:
			key = KI_RETURN;
			break;
		case VK_ESCAPE:
			key = KI_ESCAPE;
			break;
		case VK_DELETE:
			key = KI_DELETE;
			break;
		case VK_INSERT:
			key = KI_INSERT;
			break;

		}
	}

	return key;

}

bool fuse::rocket_keyboard_input_translate(Rocket::Core::Context * context, WPARAM wParam, LPARAM lParam)
{

	using namespace Rocket::Core::Input;

	bool propagate = true;

	bool keyDown = KEY_DOWN_LPARAM(lParam);

	int modifier = rocked_key_modifier();

	if (keyDown)
	{
		propagate = context->ProcessKeyDown(rocket_keyboard_map_key(wParam), modifier);
	}
	else
	{
		propagate = context->ProcessKeyUp(rocket_keyboard_map_key(wParam), modifier);
	}

	return propagate;

}