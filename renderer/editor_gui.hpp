#pragma once

#ifdef FUSE_USE_EDITOR_GUI

#include <fuse/directx_helper.hpp>

#include <Rocket/Core.h>

#include "renderable.hpp"

#include <list>

namespace fuse
{

	class editor_gui
	{

	public:

		editor_gui(void);
		editor_gui(const editor_gui &) = delete;
		editor_gui(editor_gui &&) = default;

		~editor_gui(void);
		
		bool init(void);
		void shutdown(void);

		void on_resize(UINT width, UINT height);

		LRESULT on_keyboard(WPARAM wParam, LPARAM lParam);
		LRESULT on_mouse(WPARAM wParam, LPARAM lParam);
		LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void update(void);
		void render(void);

		void select_object(renderable * object);

		//void on_mouse

	private:

		Rocket::Core::Context         * m_context;
		Rocket::Core::ElementDocument * m_selectedObject;

		std::list<Rocket::Core::ElementDocument*> m_panels;

	};

}

#endif