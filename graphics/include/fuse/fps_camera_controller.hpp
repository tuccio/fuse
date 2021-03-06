#pragma once

#include <fuse/core.hpp>
#include <fuse/scene_graph.hpp>
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
		fps_camera_controller(scene_graph_camera * camera);

		void bind_keyboard_key(keyboard_vk key, camera_action action);
		void unbind_keyboard_key(keyboard_vk key);

		void bind_mouse_key(UINT key, camera_action action);
		void unbind_mouse_key(UINT key);

		void clear_keyboard_binds(void);
		void clear_mouse_binds(void);

		void on_resize(UINT width, UINT height);
		void on_update(float dt);

		bool on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event) override;
		bool on_mouse_event(const mouse & mouse, const mouse_event_info & event) override;

		void set_camera(scene_graph_camera * camera);

	private:


		vec128 m_acceleration;
		vec128 m_velocity;

		float3 m_speed;

		float2 m_screenSize;
		float2 m_screenCenter;

		float  m_sensitivity;

		bool   m_centerMouse;
		bool   m_centeringMouse;
		bool   m_holdToRotate;

		bool   m_invertX;
		bool   m_invertY;

		bool   m_rotating;

		bool   m_strafingForward;
		bool   m_strafingBackward;
		bool   m_strafingLeft;
		bool   m_strafingRight;
		bool   m_strafingUpward;
		bool   m_strafingDownward;

		scene_graph_camera * m_camera;

		std::unordered_map<keyboard_vk, camera_action> m_keyboardBinds;
		std::unordered_map<UINT, camera_action>        m_mouseBinds;

		void handle_action_key(camera_action action, bool pressed);

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(camera,            m_camera)
			(speed,             m_speed)
			(hold_to_rotate,    m_holdToRotate)
			(auto_center_mouse, m_centerMouse)
			(sensitivity,       m_sensitivity)
			(invert_mouse_x,    m_invertX)
			(invert_mouse_y,    m_invertY)
		)

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

	};

}