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

#include <vector>
#include <unordered_map>

#include <thread>

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

	class wx_editor_gui :
		scene_graph_node_listener,
		scene_listener
	{

	public:

		wx_editor_gui(void);

		bool init(wxFrame * window, scene * scene, render_configuration * r, visual_debugger * debugger);
		void shutdown(void);

		bool is_initialized(void) const;

		scene_graph_node * get_selected_node(void) const;
		void set_selected_node(scene_graph_node * node);

		void update_scene_graph(void);
		void update_selected_object(void);
		void update_camera(void);

		void on_scene_graph_node_destruction(scene_graph_node * node) override;
		void on_scene_graph_node_move(scene_graph_node * node, const mat128 & oldTransform, const mat128 & newTransform) override;

		void on_scene_active_camera_change(scene * scene, scene_graph_camera * oldCamera, scene_graph_camera * newCamera) override;
		void on_scene_clear(scene * scene) override;

	private:

		wxFrame      * m_frame;
		wxAuiManager * m_manager;
		wxMenuBar    * m_menuBar;

		scene                * m_scene;
		render_configuration * m_rendererConfiguration;
		visual_debugger      * m_visualDebugger;

		bool m_initialized;

		// Scene graph members

		wxTreeCtrl * m_sgTree;
		std::unordered_map<scene_graph_node*, wxTreeItemId> m_nodeItemMap;
		std::unordered_map<wxTreeItemId, scene_graph_node*> m_itemNodeMap;
		scene_graph_node * m_selectedNode;
		wxTreeItemId m_draggedItem;

		// Camera members

		wxWindow * m_camera;
		
		// Private funtions

		void create_menu(void);

		void create_shadow_mapping_page(wxAuiNotebook * notebook);
		void create_skylight_page(wxAuiNotebook * notebook);
		void create_debug_page(wxAuiNotebook * notebook);
		void create_camera_page(wxAuiNotebook * notebook);
		void create_scene_graph_page(wxAuiNotebook * notebook);

		void add_branch(scene_graph_node * root);
		void remove_branch(scene_graph_node * root);

	};

}

#endif