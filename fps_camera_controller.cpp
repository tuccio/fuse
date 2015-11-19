#include <fuse/fps_camera_controller.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(fps_camera_controller, 16)

fps_camera_controller::fps_camera_controller(void) :
	m_camera(nullptr),
	m_velocity(XMVectorZero()),
	m_acceleration(XMVectorZero()),
	m_sensitivity(1.f),
	m_speed(0.25f, 0.25f, 0.25f),
	m_centerMouse(false),
	m_centeringMouse(false),
	m_holdToRotate(true),
	m_rotating(false),
	m_strafingForward(false),
	m_strafingBackward(false),
	m_strafingLeft(false),
	m_strafingRight(false),
	m_strafingUpward(false),
	m_strafingDownward(false)
{

	m_keyboardBinds = {
		{ 'W', FUSE_CAMERA_ACTION_FORWARD },
		{ 'A', FUSE_CAMERA_ACTION_LEFT },
		{ 'S', FUSE_CAMERA_ACTION_BACKWARD },
		{ 'D', FUSE_CAMERA_ACTION_RIGHT },
		{ 'E', FUSE_CAMERA_ACTION_UP },
		{ 'Q', FUSE_CAMERA_ACTION_DOWN }
	};

	m_mouseBinds = {
		{ VK_RBUTTON, FUSE_CAMERA_ACTION_ROTATE }
	};

}

fps_camera_controller::fps_camera_controller(camera * cam) :
	fps_camera_controller()
{
	m_camera = cam;
}

void fps_camera_controller::bind_keyboard_key(UINT key, camera_action action)
{
	m_keyboardBinds[key] = action;
}

void fps_camera_controller::unbind_keyboard_key(UINT key)
{

	auto it = m_keyboardBinds.find(key);

	if (it != m_keyboardBinds.end())
	{
		m_keyboardBinds.erase(it);
	}

}

void fps_camera_controller::bind_mouse_key(UINT key, camera_action action)
{
	m_mouseBinds[key] = action;
}

void fps_camera_controller::unbind_mouse_key(UINT key)
{

	auto it = m_mouseBinds.find(key);

	if (it != m_mouseBinds.end())
	{
		m_mouseBinds.erase(it);

	}
}

void fps_camera_controller::clear_keyboard_binds(void)
{
	m_keyboardBinds.clear();
}

void fps_camera_controller::clear_mouse_binds(void)
{
	m_mouseBinds.clear();
}

LRESULT fps_camera_controller::on_keyboard(WPARAM wParam, LPARAM lParam)
{

	// TODO: proper velocity/acceleration movement

	auto it = m_keyboardBinds.find(wParam);

	if (it != m_keyboardBinds.end())
	{
		handle_action_key(it->second, ((KF_UP << 16) & lParam) == 0);
	}

	return 0;

}

LRESULT fps_camera_controller::on_mouse(WPARAM wParam, LPARAM lParam)
{
	// TODO: proper velocity/acceleration movement

	if (wParam == WM_MOUSEMOVE)
	{

		if (m_centeringMouse)
		{
			m_centeringMouse = false;
			return 0;
		}

		LRESULT returnValue = 0;

		LPMOUSEHOOKSTRUCT mouseStruct = (LPMOUSEHOOKSTRUCT)lParam;

		POINT pt = mouseStruct->pt;
		ScreenToClient(mouseStruct->hwnd, &pt);

		static XMVECTOR lastPos = fuse::to_vector(XMFLOAT2(pt.x, pt.y));
		XMVECTOR to = fuse::to_vector(XMFLOAT2(pt.x, pt.y));

		if (m_rotating)
		{

			XMVECTOR from = lastPos;

			XMVECTOR delta = XMVectorATan((to - from) / fuse::to_vector(m_screenSize));

			XMVECTOR r1 = XMQuaternionRotationAxis(to_vector(XMFLOAT3(0, 1, 0)), XMVectorGetX(delta));
			XMVECTOR r2 = XMQuaternionRotationAxis(to_vector(XMFLOAT3(1, 0, 0)), XMVectorGetY(delta));

			XMVECTOR newOrientation = XMQuaternionNormalize(XMQuaternionMultiply(XMQuaternionMultiply(r2, m_camera->get_orientation()), r1));

			m_camera->set_orientation(newOrientation);

			returnValue = 1;

		}

		if (m_centerMouse)
		{

			lastPos = to_vector(m_screenCenter);
			m_centeringMouse = true;

			POINT screenCenter = { m_screenCenter.x, m_screenCenter.y };

			ClientToScreen(mouseStruct->hwnd, &screenCenter);
			SetCursorPos(screenCenter.x, screenCenter.y);

		}
		else
		{
			lastPos = to;
		}

		return returnValue;
		
	}
	else
	{

		UINT vk = 0;
		bool pressed;

		switch (wParam)
		{
		case WM_RBUTTONDOWN:
			vk = VK_RBUTTON;
			pressed = true;
			break;
		case WM_RBUTTONUP:
			vk = VK_RBUTTON;
			pressed = false;
			break;
		case WM_LBUTTONDOWN:
			vk = VK_LBUTTON;
			pressed = true;
			break;
		case WM_LBUTTONUP:
			vk = VK_LBUTTON;
			pressed = false;
			break;
		case WM_MBUTTONDOWN:
			vk = VK_MBUTTON;
			pressed = true;
			break;
		case WM_MBUTTONUP:
			vk = VK_MBUTTON;
			pressed = false;
			break;
		}
		
		if (vk)
		{

			auto it = m_mouseBinds.find(vk);

			if (it != m_mouseBinds.end())
			{
				handle_action_key(it->second, pressed);
			}

		}

		// TODO: mouse binds

	}

	return 0;

}

LRESULT fps_camera_controller::on_resize(UINT width, UINT height)
{
	m_screenSize   = XMFLOAT2(width, height);
	m_screenCenter = XMFLOAT2(width / 2, height / 2);
	return 0;
}

LRESULT fps_camera_controller::on_update(float dt)
{

	// TODO: RK4 solver

	if (m_strafingForward)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position + m_camera->forward() * m_speed.z * dt);
	}

	if (m_strafingBackward)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position - m_camera->forward() * m_speed.z * dt);
	}

	if (m_strafingLeft)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position - m_camera->right() * m_speed.x * dt);
	}

	if (m_strafingRight)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position + m_camera->right() * m_speed.x * dt);
	}

	if (m_strafingUpward)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position + m_camera->up() * m_speed.y * dt);
	}

	if (m_strafingDownward)
	{
		auto position = m_camera->get_position();
		m_camera->set_position(position - m_camera->up() * m_speed.y * dt);
	}

	return 0;

}

void fps_camera_controller::handle_action_key(camera_action action, bool pressed)
{
	switch (action)
	{
	case FUSE_CAMERA_ACTION_FORWARD:
		m_strafingForward = pressed;
		break;
	case FUSE_CAMERA_ACTION_BACKWARD:
		m_strafingBackward = pressed;
		break;
	case FUSE_CAMERA_ACTION_LEFT:
		m_strafingLeft = pressed;
		break;
	case FUSE_CAMERA_ACTION_RIGHT:
		m_strafingRight = pressed;
		break;
	case FUSE_CAMERA_ACTION_UP:
		m_strafingUpward = pressed;
		break;
	case FUSE_CAMERA_ACTION_DOWN:
		m_strafingDownward = pressed;
		break;
	case FUSE_CAMERA_ACTION_ROTATE:
		m_rotating = pressed;
		break;
	}
}