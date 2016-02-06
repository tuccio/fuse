#pragma once

#include <fuse/allocators.hpp>
#include <fuse/camera.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/keyboard.hpp>
#include <fuse/mouse.hpp>

#include <unordered_map>

enum camera_action
{
	FUSE_CAMERA_ACTION_FORWARD,
	FUSE_CAMERA_ACTION_BACKWARD,
	FUSE_CAMERA_ACTION_LEFT,
	FUSE_CAMERA_ACTION_RIGHT,
	FUSE_CAMERA_ACTION_UP,
	FUSE_CAMERA_ACTION_DOWN,
	FUSE_CAMERA_ACTION_ROTATE
};

namespace fuse
{

	class alignas(16) fps_camera_controller :
		public keyboard_listener,
		public mouse_listener
	{

	public:

		fps_camera_controller(void);
		fps_camera_controller(camera * cam);

		void bind_keyboard_key(keyboard_vk key, camera_action action);
		void unbind_keyboard_key(keyboard_vk key);

		void bind_mouse_key(UINT key, camera_action action);
		void unbind_mouse_key(UINT key);

		void clear_keyboard_binds(void);
		void clear_mouse_binds(void);

		LRESULT on_keyboard(WPARAM wParam, LPARAM lParam);
		LRESULT on_mouse(WPARAM wParam, LPARAM lParam);

		LRESULT on_resize(UINT width, UINT height);

		LRESULT on_update(float dt);

		bool on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event) override;
		bool on_mouse_event(const mouse & mouse, const mouse_event_info & event) override;

	private:

		XMVECTOR   m_acceleration;
		XMVECTOR   m_velocity;

		camera   * m_camera;

		XMFLOAT3   m_speed;

		XMFLOAT2   m_screenSize;
		XMFLOAT2   m_screenCenter;

		float      m_sensitivity;

		bool       m_centerMouse;
		bool       m_centeringMouse;
		bool       m_holdToRotate;

		bool       m_rotating;

		bool       m_strafingForward;
		bool       m_strafingBackward;
		bool       m_strafingLeft;
		bool       m_strafingRight;
		bool       m_strafingUpward;
		bool       m_strafingDownward;


		std::unordered_map<keyboard_vk, camera_action> m_keyboardBinds;
		std::unordered_map<UINT, camera_action> m_mouseBinds;

		void handle_action_key(camera_action action, bool pressed);

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(camera,            m_camera)
			(speed,             m_speed)
			(hold_to_rotate,    m_holdToRotate)
			(auto_center_mouse, m_centerMouse)
			(sensitivity,       m_sensitivity)
		)

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

	};

}