#include <fuse/fps_camera_controller.hpp>

using namespace fuse;

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(fps_camera_controller, 16)

fps_camera_controller::fps_camera_controller(void) :
	m_camera(nullptr),
	m_velocity(vec128_zero()),
	m_acceleration(vec128_zero()),
	m_sensitivity(1.f),
	m_speed(0.25f, 0.25f, 0.25f),
	m_centerMouse(false),
	m_centeringMouse(false),
	m_holdToRotate(true),
	m_invertX(false),
	m_invertY(false),
	m_rotating(false),
	m_strafingForward(false),
	m_strafingBackward(false),
	m_strafingLeft(false),
	m_strafingRight(false),
	m_strafingUpward(false),
	m_strafingDownward(false)
{

	m_keyboardBinds = {
		{ FUSE_KEYBOARD_VK_W, FUSE_CAMERA_ACTION_FORWARD },
		{ FUSE_KEYBOARD_VK_A, FUSE_CAMERA_ACTION_LEFT },
		{ FUSE_KEYBOARD_VK_S, FUSE_CAMERA_ACTION_BACKWARD },
		{ FUSE_KEYBOARD_VK_D, FUSE_CAMERA_ACTION_RIGHT },
		{ FUSE_KEYBOARD_VK_E, FUSE_CAMERA_ACTION_UP },
		{ FUSE_KEYBOARD_VK_Q, FUSE_CAMERA_ACTION_DOWN }
	};

	m_mouseBinds = {
		{ VK_RBUTTON, FUSE_CAMERA_ACTION_ROTATE }
	};

}

fps_camera_controller::fps_camera_controller(scene_graph_camera * camera) :
	fps_camera_controller()
{
	set_camera(camera);
}

void fps_camera_controller::bind_keyboard_key(keyboard_vk key, camera_action action)
{
	m_keyboardBinds[key] = action;
}

void fps_camera_controller::unbind_keyboard_key(keyboard_vk key)
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

void fps_camera_controller::on_resize(UINT width, UINT height)
{
	m_screenSize   = float2(width, height);
	m_screenCenter = float2(width / 2, height / 2);
}

void fps_camera_controller::on_update(float dt)
{
	// TODO: RK4 solver

	if (m_camera)
	{
		const vec128 f = to_vec128(m_camera->get_camera()->forward());
		const vec128 r = to_vec128(m_camera->get_camera()->right());
		const vec128 u = to_vec128(m_camera->get_camera()->up());

		if (m_strafingForward)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position + (m_speed.z * dt) * f;
			m_camera->set_local_translation(newPosition);
		}

		if (m_strafingBackward)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position - (m_speed.z * dt) * f;
			m_camera->set_local_translation(newPosition);
		}

		if (m_strafingLeft)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position - (m_speed.x * dt) * r;
			m_camera->set_local_translation(newPosition);
		}

		if (m_strafingRight)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position + (m_speed.x * dt) * r;
			m_camera->set_local_translation(newPosition);
		}

		if (m_strafingUpward)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position + (m_speed.x * dt) * u;
			m_camera->set_local_translation(newPosition);
		}

		if (m_strafingDownward)
		{
			vec128 position = m_camera->get_local_translation();
			vec128 newPosition = position - (m_speed.x * dt) * u;
			m_camera->set_local_translation(newPosition);
		}
	}
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

bool fps_camera_controller::on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event)
{
	auto it = m_keyboardBinds.find(event.key);

	if (it != m_keyboardBinds.end())
	{
		handle_action_key(it->second, event.type != FUSE_KEYBOARD_EVENT_BUTTON_UP);
	}

	return false;
}

bool fps_camera_controller::on_mouse_event(const mouse & mouse, const mouse_event_info & event)
{
	if (m_camera)
	{
		if (event.type == FUSE_MOUSE_EVENT_MOVE)
		{
			/*if (m_centeringMouse)
			{
			m_centeringMouse = false;
			return 0;
			}*/

			int2 pt = event.position;

			static float2 lastPos(pt.x, pt.y);
			float2 to(pt.x, pt.y);

			if (m_rotating)
			{
				float2 from = lastPos;
				float2 distance = to - from;
				float2 t = distance / float2(m_screenSize.x, m_screenSize.y);
				float2 delta(std::atan(t.x), std::atan(t.y));

				quaternion o = to_quaternion(m_camera->get_local_rotation());

				quaternion r1(float3(0, 1, 0), m_invertX ? -delta.x : delta.x);
				quaternion r2(float3(1, 0, 0), m_invertY ? -delta.y : delta.y);

				quaternion newOrientation = normalize(r2 * o * r1);

				m_camera->set_local_rotation(newOrientation);
			}

			if (m_centerMouse)
			{
				/*lastPos = to_vector(m_screenCenter);
				m_centeringMouse = true;

				POINT screenCenter = { m_screenCenter.x, m_screenCenter.y };

				ClientToScreen(mouseStruct->hwnd, &screenCenter);
				SetCursorPos(screenCenter.x, screenCenter.y);*/
			}
			else
			{
				lastPos = to;
			}
		}
		else if (event.type == FUSE_MOUSE_EVENT_BUTTON_DOWN)
		{
			auto it = m_mouseBinds.find(event.key);

			if (it != m_mouseBinds.end())
			{
				handle_action_key(it->second, true);
			}
		}
		else if (event.type == FUSE_MOUSE_EVENT_BUTTON_UP)
		{
			auto it = m_mouseBinds.find(event.key);

			if (it != m_mouseBinds.end())
			{
				handle_action_key(it->second, FALSE);
			}
		}
	}

	return false;
}