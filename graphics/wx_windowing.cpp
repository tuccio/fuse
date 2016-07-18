#ifdef FUSE_WXWIDGETS

#include <fuse/wx_windowing.hpp>
#include <fstream>
#include <string>

using namespace fuse;
using namespace fuse::wx;

#define FUSE_WX_WINDOWING_VAR(Type, Name) Type wx_windowing::Name;

FUSE_WX_WINDOWING_VAR(HINSTANCE, m_hInstance)
FUSE_WX_WINDOWING_VAR(HWND, m_hWndRenderWindow)

FUSE_WX_WINDOWING_VAR(int, m_renderWidth)
FUSE_WX_WINDOWING_VAR(int, m_renderHeight)

FUSE_WX_WINDOWING_VAR(bool, m_silent)
FUSE_WX_WINDOWING_VAR(bool, m_initialized)
FUSE_WX_WINDOWING_VAR(bool, m_resized)

FUSE_WX_WINDOWING_VAR(wx_window *, m_mainWindow)

FUSE_WX_WINDOWING_VAR(keyboard, m_keyboard)
FUSE_WX_WINDOWING_VAR(mouse,    m_mouse)

#define FUSE_WINDOW_MIN_SIZE_X 320
#define FUSE_WINDOW_MIN_SIZE_Y 240

bool wx_windowing::init(HINSTANCE hInstance, bool silent)
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

void wx_windowing::shutdown(void)
{
	if (m_initialized)
	{
		delete m_mainWindow;

		wxEntryCleanup();
		wxUninitialize();

		m_initialized = false;
	}
}

bool wx_windowing::create_window(int width, int height, const char_t * caption)
{
	m_mainWindow = new wx_window(caption, wxPoint(100, 100), wxSize(width, height));
	return m_mainWindow->Show();
}

void wx_windowing::destroy_window(void)
{
	m_mainWindow->Close(true);
}

void wx_windowing::signal_error(const char_t * error)
{
	if (!m_silent)
	{
		wxMessageBox(error, FUSE_LITERAL("Fuse Error"));
	}
}

void wx_windowing::add_keyboard_callback(keyboard_callback callback, unsigned int priority)
{
	m_keyboard.add_callback(callback, priority);
}

void wx_windowing::remove_keyboard_callback(keyboard_callback callback)
{
	m_keyboard.remove_callback(callback);
}

void wx_windowing::add_keyboard_listener(keyboard_listener * listener, unsigned int priority)
{
	m_keyboard.add_listener(listener, priority);
}

void wx_windowing::remove_keyboard_listener(keyboard_listener * listener)
{
	m_keyboard.remove_listener(listener);
}

void wx_windowing::add_mouse_callback(mouse_callback callback, unsigned int priority)
{
	m_mouse.add_callback(callback, priority);
}

void wx_windowing::remove_mouse_callback(mouse_callback callback)
{
	m_mouse.remove_callback(callback);
}

void wx_windowing::add_mouse_listener(mouse_listener * listener, unsigned int priority)
{
	m_mouse.add_listener(listener, priority);
}

void wx_windowing::remove_mouse_listener(mouse_listener * listener)
{
	m_mouse.remove_listener(listener);
}

void wx_windowing::update_windows(void)
{
	wxTheApp->ProcessIdle();
}

void wx_windowing::maximize(void)
{
	m_mainWindow->Maximize();
}

void wx_windowing::load_ui_configuration(const char * filename)
{
	m_mainWindow->load_aui_conf(filename);
}

void wx_windowing::save_ui_configuration(const char * filename)
{
	m_mainWindow->save_aui_conf(filename);
}

void wx_windowing::quit(void)
{
	// We don't really use wxWidgets main loop
	PostQuitMessage(0);
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
	m_manager.AddPane(m_renderPanel, wxCENTER);

	m_manager.GetPane(m_renderPanel).name = "wx_window::render_panel";

	m_manager.Update();

}

wx_window::~wx_window(void)
{
	m_manager.UnInit();
}

void wx_window::create_render_panel(void)
{
	m_renderPanel = new wx_render_panel(this);
	wx_windowing::m_hWndRenderWindow = m_renderPanel->GetHWND();
}

void wx_window::on_close(wxCloseEvent &)
{
	PostQuitMessage(0);
}

bool wx_window::load_aui_conf(const char * filename)
{

	std::wifstream f(filename);

	if (f)
	{

		std::wstring content;
		f >> content;

		wxString perspective = content;
		m_manager.LoadPerspective(perspective);

		return true;

	}

	return false;

}

bool wx_window::save_aui_conf(const char * filename)
{

	std::wofstream f(filename);

	if (f)
	{
		f << m_manager.SavePerspective().utf8_str().data();
		return true;
	}

	return false;

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

#endif