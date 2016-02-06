#include <fuse/base_windowing.hpp>
#include <fuse/logger.hpp>
#include <fuse/directx_helper.hpp>

#define FUSE_WINDOW_CLASS FUSE_LITERAL("fuse_window_class")

using namespace fuse;

#define FUSE_WINDOW_MIN_SIZE_X 320
#define FUSE_WINDOW_MIN_SIZE_Y 240

#define FUSE_BASE_WINDOWING_VAR(Type, Name) Type base_windowing::Name;

FUSE_BASE_WINDOWING_VAR(HINSTANCE, m_hInstance)
FUSE_BASE_WINDOWING_VAR(HWND, m_hWnd)

FUSE_BASE_WINDOWING_VAR(int, m_clientWidth)
FUSE_BASE_WINDOWING_VAR(int, m_clientHeight)

FUSE_BASE_WINDOWING_VAR(bool, m_silent)
FUSE_BASE_WINDOWING_VAR(bool, m_initialized)
FUSE_BASE_WINDOWING_VAR(bool, m_resized)

/* Member functions */

/* Window class and callbacks */

LRESULT CALLBACK base_windowing::window_proc(HWND hWnd,
                                             UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam)
{

	switch (uMsg)
	{

	case WM_DESTROY:

		PostQuitMessage(0);
		break;

	case WM_SIZE:

	{

		if (wParam == SIZE_MINIMIZED)
		{
			// Maybe sleep
		}
		else
		{

			RECT clientRect;
			GetClientRect(hWnd, &clientRect);

			m_clientWidth  = clientRect.right - clientRect.left;
			m_clientHeight = clientRect.bottom - clientRect.top;

			m_resized = true;

		}

		break;

	}

	break;


	case WM_GETMINMAXINFO:

	{

		MINMAXINFO * minMaxInfo = reinterpret_cast<MINMAXINFO *>(lParam);

		minMaxInfo->ptMinTrackSize.x = FUSE_WINDOW_MIN_SIZE_X;
		minMaxInfo->ptMinTrackSize.y = FUSE_WINDOW_MIN_SIZE_Y;

		return 0;

	}

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);

}

bool base_windowing::init(HINSTANCE hInstance, bool silent)
{

	if (!m_initialized)
	{

		m_hInstance = hInstance;

		WNDCLASSEX wndClass = { 0 };

		wndClass.cbSize        = sizeof(WNDCLASSEX);
		wndClass.style         = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc   = window_proc;
		wndClass.hInstance     = hInstance;
		wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wndClass.lpszClassName = FUSE_WINDOW_CLASS;

		if (!(m_initialized = RegisterClassEx(&wndClass)))
		{
			FUSE_LOG_OPT_DEBUG(FUSE_LITERAL("Failed to register window class."));
		}

	}
	
	return m_initialized;

}

void base_windowing::shutdown(void)
{

	if (m_initialized)
	{

		destroy_window();

		UnregisterClass(FUSE_WINDOW_CLASS, m_hInstance);
		m_initialized = false;

	}

}

bool base_windowing::create_window(int width, int height, const char_t * caption)
{

	assert(m_hInstance && m_initialized && "Cannot create window before initializing.");
	assert(!m_hWnd && "Window already exists.");

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hWnd = CreateWindowEx(0,
		FUSE_WINDOW_CLASS,
		caption,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		m_hInstance,
		nullptr);

	if (!m_hWnd)
	{
		FUSE_HR_LOG(GetLastError());
		return false;
	}
	else
	{
		ShowWindow(m_hWnd, SW_NORMAL);
		return true;
	}

}

void base_windowing::destroy_window(void)
{
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}

LRESULT CALLBACK base_windowing::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK base_windowing::on_mouse(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

void base_windowing::signal_error(const char_t * error)
{
	if (!m_silent)
	{
		MessageBox(get_render_window(), error, FUSE_LITERAL("Fuse error"), MB_ICONERROR);
	}
}