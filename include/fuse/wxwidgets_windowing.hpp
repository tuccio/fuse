#pragma once

#ifdef FUSE_WXWIDGETS

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/aui/aui.h>

#include <fuse/logger.hpp>
#include <fuse/input.hpp>
#include <fuse/types.hpp>

#include <memory>
#include <thread>

namespace fuse
{

	struct wxwidgets_windowing;

	namespace wx
	{

		class wx_app :
			public wxApp
		{

		public:

			bool OnInit(void) override { return true; }

		};

		class wx_render_panel :
			public wxPanel
		{

		public:

			wx_render_panel(
				wxWindow * parent,
				wxWindowID id = wxID_ANY,
				const wxPoint & pos = wxDefaultPosition,
				const wxSize & size = wxDefaultSize,
				long style = wxTAB_TRAVERSAL,
				const wxString & name = wxPanelNameStr);

		private:

			inline void on_resize(wxSizeEvent & event);
			inline void on_keyboard_key_down(wxKeyEvent & event);
			inline void on_keyboard_key_up(wxKeyEvent & event);
			inline void on_mouse_event(wxMouseEvent & event);

			wxDECLARE_EVENT_TABLE();

		};

		class wx_window :
			public wxFrame
		{

		public:

			wx_window(const wxString & title, const wxPoint & pos, const wxSize & size);			
			~wx_window(void);

			void create_render_panel(void);
			wx_render_panel * get_render_panel(void) const { return m_renderPanel.get(); }

		private:

			inline void on_resize(wxSizeEvent & event);
			void on_close(wxCloseEvent & event);

			std::unique_ptr<wx_render_panel> m_renderPanel;

			wxAuiManager m_manager;

			wxDECLARE_EVENT_TABLE();

		};

	}

	struct wxwidgets_windowing
	{

	public:

		wxwidgets_windowing(void) = delete;

		static bool init(HINSTANCE hInstance, bool silent = false);
		static void shutdown(void);

		inline static LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }

		static bool create_window(int width, int height, const char_t * caption);
		static void destroy_window(void);

		inline static HINSTANCE get_instance(void) { return m_hInstance; }
		inline static HWND get_render_window(void) { return m_hWndRenderWindow; }

		inline static int get_render_window_width(void) { return m_renderWidth; }
		inline static int get_render_window_height(void) { return m_renderHeight; }

		inline static bool get_resize_polling(void) { bool resized = m_resized; m_resized = false; return resized; }

		static void signal_error(const char_t * error);

		static void update_windows(void);

	protected:

		static void add_keyboard_callback(keyboard_callback callback, unsigned int priority = FUSE_PRIORITY_DEFAULT);
		static void remove_keyboard_callback(keyboard_callback callback);

		static void add_keyboard_listener(keyboard_listener * listener, unsigned int priority = FUSE_PRIORITY_DEFAULT);
		static void remove_keyboard_listener(keyboard_listener * listener);

		static void add_mouse_callback(mouse_callback callback, unsigned int priority = FUSE_PRIORITY_DEFAULT);
		static void remove_mouse_callback(mouse_callback callback);

		static void add_mouse_listener(mouse_listener * listener, unsigned int priority = FUSE_PRIORITY_DEFAULT);
		static void remove_mouse_listener(mouse_listener * listener);

		inline static keyboard & get_keyboard(void) { return m_keyboard; }
		inline static mouse & get_mouse(void) { return m_mouse; }

		inline static wxAuiManager * get_wx_aui_manager(void) { return wxAuiManager::GetManager(m_mainWindow); }

	private:

		static HINSTANCE m_hInstance;
		static HWND      m_hWndRenderWindow;

		static int m_renderWidth;
		static int m_renderHeight;

		static bool m_silent;
		static bool m_initialized;
		static bool m_resized;

		static wx::wx_window * m_mainWindow;

		static keyboard m_keyboard;
		static mouse    m_mouse;

		static void resize(int x, int y)
		{
			m_renderWidth  = x;
			m_renderHeight = y;				 
			m_resized      = true;
		}

		friend class wx::wx_window;
		friend class wx::wx_render_panel;

	};

	namespace wx
	{

		void wx_window::on_resize(wxSizeEvent & event)
		{

			wxSize clientSize = m_renderPanel->GetClientSize();

			wxwidgets_windowing::m_resized      = true;
			wxwidgets_windowing::m_renderWidth  = clientSize.x;
			wxwidgets_windowing::m_renderHeight = clientSize.y;

		}

		void wx_render_panel::on_resize(wxSizeEvent & event)
		{
			wxSize size = event.GetSize();
			wxwidgets_windowing::resize(size.x, size.y);
		}

		void wx_render_panel::on_keyboard_key_down(wxKeyEvent & event)
		{

			keyboard_vk key = keyboard_vk_from_win32(event.GetRawKeyCode());

			if (key != FUSE_KEYBOARD_VK_UNKNOWN)
			{
				wxwidgets_windowing::get_keyboard().post_keyboard_button_down(key);
			}
			
		}

		void wx_render_panel::on_keyboard_key_up(wxKeyEvent & event)
		{
			
			keyboard_vk key = keyboard_vk_from_win32(event.GetRawKeyCode());

			if (key != FUSE_KEYBOARD_VK_UNKNOWN)
			{
				wxwidgets_windowing::get_keyboard().post_keyboard_button_up(key);
			}

		}

		void wx_render_panel::on_mouse_event(wxMouseEvent & event)
		{

			if (event.Moving() || event.Dragging())
			{
				wxwidgets_windowing::get_mouse().post_mouse_move(reinterpret_cast<const XMINT2&>(event.GetPosition()));
			}
			if (event.LeftDown())
			{
				SetFocus();
				wxwidgets_windowing::get_mouse().post_mouse_button_down(FUSE_MOUSE_VK_MOUSE1);
			}
			else if (event.LeftUp())
			{
				wxwidgets_windowing::get_mouse().post_mouse_button_up(FUSE_MOUSE_VK_MOUSE1);
			}
			else if (event.RightDown())
			{
				SetFocus();
				wxwidgets_windowing::get_mouse().post_mouse_button_down(FUSE_MOUSE_VK_MOUSE2);
			}
			else if (event.RightUp())
			{
				wxwidgets_windowing::get_mouse().post_mouse_button_up(FUSE_MOUSE_VK_MOUSE2);
			}
			else if (event.MiddleDown())
			{
				SetFocus();
				wxwidgets_windowing::get_mouse().post_mouse_button_down(FUSE_MOUSE_VK_MOUSE3);
			}
			else if (event.MiddleUp())
			{
				wxwidgets_windowing::get_mouse().post_mouse_button_up(FUSE_MOUSE_VK_MOUSE3);
			}

		}

	}

	

}

#endif
