#pragma once

#ifdef FUSE_WXWIDGETS

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/aui/aui.h>
#include <wx/treectrl.h>

#include "render_variables.hpp"
#include "scene.hpp"
#include "visual_debugger.hpp"

#include <unordered_map>

namespace std
{
	template <>
	struct hash<wxTreeItemId>
	{
		size_t operator()(const wxTreeItemId & lhs) const
		{
			return std::hash<const void*>()(lhs.GetID());
		}
	};
}

namespace fuse
{

	class wx_editor_gui
	{

	public:

		bool init(wxWindow * window, scene * scene, render_configuration * r, visual_debugger * debugger);
		void shutdown(void);

		scene_graph_node * get_selected_node(void) const;
		void set_selected_node(scene_graph_node * node);

	private:

		wxWindow     * m_window;
		wxAuiManager * m_manager;

		scene                * m_scene;
		render_configuration * m_rendererConfiguration;
		visual_debugger      * m_visualDebugger;

		// Scene graph members

		wxTreeCtrl * m_sgTree;
		std::unordered_map<scene_graph_node*, wxTreeItemId> m_nodeItemMap;
		std::unordered_map<wxTreeItemId, scene_graph_node*> m_itemNodeMap;
		scene_graph_node * m_selectedNode;
		
		// Private funtions

		void create_shadow_mapping_page(wxAuiNotebook * notebook);
		void create_skylight_page(wxAuiNotebook * notebook);
		void create_debug_page(wxAuiNotebook * notebook);
		void create_camera_page(wxAuiNotebook * notebook);
		void create_scene_graph_page(wxAuiNotebook * notebook);

		void load_scene_graph(void);

	};

}

#endif