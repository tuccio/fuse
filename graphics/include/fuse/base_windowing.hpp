#pragma once

#include <fuse/core.hpp>
#include <Windows.h>

namespace fuse
{

	struct base_windowing
	{

	public:

		base_windowing(void) = delete;

		static bool init(HINSTANCE hInstance, bool silent = false);
		static void shutdown(void);

		static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		inline static LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }

		static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam);

		static bool create_window(int width, int height, const char_t * caption);
		static void destroy_window(void);

		inline static HINSTANCE get_instance(void) { return m_hInstance; }
		inline static HWND get_render_window(void) { return m_hWnd; }

		inline static int get_render_window_width(void) { return m_clientWidth; }
		inline static int get_render_window_height(void) { return m_clientHeight; }

		inline static bool get_resize_polling(void) { bool resized = m_resized; m_resized = false; return resized; }

		static void signal_error(const char_t * error);

		static void quit(void);

	private:

		static HINSTANCE m_hInstance;
		static HWND      m_hWnd;

		static int m_clientWidth;
		static int m_clientHeight;

		static bool m_silent;
		static bool m_initialized;

		static bool m_resized;

	};

}