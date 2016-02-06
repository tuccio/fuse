#include <fuse/wxwidgets_windowing.hpp>

using namespace fuse;
using namespace fuse::wx;

#define FUSE_WXWIDGETS_WINDOWING_VAR(Type, Name) Type wxwidgets_windowing::Name;

FUSE_WXWIDGETS_WINDOWING_VAR(HINSTANCE, m_hInstance)
FUSE_WXWIDGETS_WINDOWING_VAR(HWND, m_hWndRenderWindow)

FUSE_WXWIDGETS_WINDOWING_VAR(int, m_renderWidth)
FUSE_WXWIDGETS_WINDOWING_VAR(int, m_renderHeight)

FUSE_WXWIDGETS_WINDOWING_VAR(bool, m_silent)
FUSE_WXWIDGETS_WINDOWING_VAR(bool, m_initialized)
FUSE_WXWIDGETS_WINDOWING_VAR(bool, m_resized)

FUSE_WXWIDGETS_WINDOWING_VAR(wx_window *, m_mainWindow)

FUSE_WXWIDGETS_WINDOWING_VAR(keyboard, m_keyboard)
FUSE_WXWIDGETS_WINDOWING_VAR(mouse,    m_mouse)

#define FUSE_WINDOW_MIN_SIZE_X 320
#define FUSE_WINDOW_MIN_SIZE_Y 240

bool wxwidgets_windowing::init(HINSTANCE hInstance, bool silent)
{

	if (!m_initialized)
	{

		m_initialized = wxInitialize();
		m_hInstance   = hInstance;
		m_silent      = silent;

		wxApp::SetInstance(new wx_app);

		wxEntryStart(hInstance);

	}

	return m_initialized;

}

void wxwidgets_windowing::shutdown(void)
{

	if (m_initialized)
	{

		wxEntryCleanup();

		wxUninitialize();

		m_initialized = false;

	}

}

bool wxwidgets_windowing::create_window(int width, int height, const char_t * caption)
{
	
	m_mainWindow = new wx_window(caption, wxPoint(100, 100), wxSize(width, height));
	

	return m_mainWindow->Show();

}

void wxwidgets_windowing::destroy_window(void)
{
	m_mainWindow->Close(true);
}

void wxwidgets_windowing::signal_error(const char_t * error)
{
	if (!m_silent)
	{
		wxMessageBox(error, FUSE_LITERAL("Fuse Error"));
	}
}

void wxwidgets_windowing::add_keyboard_callback(keyboard_callback callback, unsigned int priority)
{
	m_keyboard.add_callback(callback, priority);
}

void wxwidgets_windowing::remove_keyboard_callback(keyboard_callback callback)
{
	m_keyboard.remove_callback(callback);
}

void wxwidgets_windowing::add_keyboard_listener(keyboard_listener * listener, unsigned int priority)
{
	m_keyboard.add_listener(listener, priority);
}

void wxwidgets_windowing::remove_keyboard_listener(keyboard_listener * listener)
{
	m_keyboard.remove_listener(listener);
}

void wxwidgets_windowing::add_mouse_callback(mouse_callback callback, unsigned int priority)
{
	m_mouse.add_callback(callback, priority);
}

void wxwidgets_windowing::remove_mouse_callback(mouse_callback callback)
{
	m_mouse.remove_callback(callback);
}

void wxwidgets_windowing::add_mouse_listener(mouse_listener * listener, unsigned int priority)
{
	m_mouse.add_listener(listener, priority);
}

void wxwidgets_windowing::remove_mouse_listener(mouse_listener * listener)
{
	m_mouse.remove_listener(listener);
}

void wxwidgets_windowing::update_windows(void)
{
	wxTheApp->ProcessIdle();
}

/* Main window */

wxBEGIN_EVENT_TABLE(wx_window, wxFrame)
	EVT_SIZE(wx_window::on_resize)
	EVT_CLOSE(wx_window::on_close)
wxEND_EVENT_TABLE()

wx_window::wx_window(const wxString & title, const wxPoint & pos, const wxSize & size) :
	wxFrame(nullptr, wxID_ANY, title, pos, size),
	m_renderPanel(nullptr)
{

	SetMinClientSize(wxSize(FUSE_WINDOW_MIN_SIZE_X, FUSE_WINDOW_MIN_SIZE_Y));

	create_render_panel();

	m_manager.SetManagedWindow(this);
	m_manager.AddPane(m_renderPanel.get(), wxCENTER);

	m_manager.Update();

}

wx_window::~wx_window(void)
{
	m_manager.UnInit();
}

void wx_window::create_render_panel(void)
{
	m_renderPanel = std::make_unique<wx_render_panel>(this);
	wxwidgets_windowing::m_hWndRenderWindow = m_renderPanel->GetHWND();
}

void wx_window::on_close(wxCloseEvent &)
{
	PostQuitMessage(0);
}

wxBEGIN_EVENT_TABLE(wx_render_panel, wxPanel)
	EVT_SIZE(wx_render_panel::on_resize)
	EVT_KEY_DOWN(wx_render_panel::on_keyboard_key_down)
	EVT_KEY_UP(wx_render_panel::on_keyboard_key_up)
	EVT_MOUSE_EVENTS(wx_render_panel::on_mouse_event)
wxEND_EVENT_TABLE()

/* Render panel */

wx_render_panel::wx_render_panel(
	wxWindow * parent,
	wxWindowID id,
	const wxPoint & pos,
	const wxSize & size,
	long style,
	const wxString & name) :
	wxPanel(parent, id, pos, size, style, name)
{
	SetMinClientSize(wxSize(FUSE_WINDOW_MIN_SIZE_X, FUSE_WINDOW_MIN_SIZE_Y));
}
