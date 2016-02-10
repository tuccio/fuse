#pragma once

#ifdef FUSE_WXWIDGETS

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/aui/aui.h>

#include "render_variables.hpp"
#include "scene.hpp"
#include "visual_debugger.hpp"

namespace fuse
{

	class wx_editor_gui
	{

	public:

		bool init(wxWindow * window, scene * scene, renderer_configuration * r, visual_debugger * debugger);
		void shutdown(void);

	private:

		wxWindow     * m_window;
		wxAuiManager * m_manager;


		scene * m_scene;
		renderer_configuration * m_rendererConfiguration;
		visual_debugger * m_visualDebugger;

		void create_shadow_mapping_page(wxAuiNotebook * notebook);
		void create_skylight_page(wxAuiNotebook * notebook);
		void create_debug_page(wxAuiNotebook * notebook);
		void create_camera_page(wxAuiNotebook * notebook);

	};

}

#endif