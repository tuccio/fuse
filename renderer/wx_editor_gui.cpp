#ifdef FUSE_WXWIDGETS

#define FUSE_WX_SLIDER(Parent, Type, Get, Set, Min, Max) new wx_slider<Type>([&](Type x) { Set(x); }, Parent, wxID_ANY, Get(), Min, Max)
#define FUSE_WX_SPIN(Parent, Type, Get, Set, Min, Max, Step) new wx_spin<Type>([&](Type x) { Set(x); }, Parent, wxID_ANY, Get(), Min, Max, Step)

#define FUSE_WX_SLIDER_RV(Parent, Type, Variable, Min, Max) FUSE_WX_SLIDER(Parent, Type, m_rendererConfiguration->get_ ## Variable, m_rendererConfiguration->set_ ## Variable, Min, Max)
#define FUSE_WX_SPIN_RV(Parent, Type, Variable, Min, Max, Step) FUSE_WX_SPIN(Parent, Type, m_rendererConfiguration->get_ ## Variable, m_rendererConfiguration->set_ ## Variable, Min, Max, Step)

#define FUSE_RENDERING_NOTEBOOK _("wx_editor_gui::rendering_notebook")
#define FUSE_SCENE_NOTEBOOK     _("wx_editor_gui::scene_notebook")

#include "renderer_application.hpp"
#include "wx_editor_gui.hpp"

#include <fuse/wx_helpers.hpp>

#include <algorithm>
#include <deque>
#include <iterator>

wxDEFINE_EVENT(FUSE_EVENT_REFRESH, wxCommandEvent);

using namespace fuse;

wx_editor_gui::wx_editor_gui(void) :
	m_initialized(false) {}

bool wx_editor_gui::init(wxFrame * frame, scene * scene, render_configuration * r, visual_debugger * visualDebugger)
{
	m_initialized = false;

	m_frame   = frame;
	m_manager = wxAuiManager::GetManager(frame);
	m_scene   = scene;

	m_visualDebugger        = visualDebugger;
	m_rendererConfiguration = r;

	m_selectedNode = nullptr;

	create_menu();

	wxAuiNotebook * renderingNotebook = new wxAuiNotebook(frame, wxID_ANY);
	wxAuiNotebook * sceneNotebook     = new wxAuiNotebook(frame, wxID_ANY);

	renderingNotebook->Freeze();
	sceneNotebook->Freeze();

	create_shadow_mapping_page(renderingNotebook);
	create_skylight_page(sceneNotebook);
	create_camera_page(sceneNotebook);
	create_scene_graph_page(sceneNotebook);

	if (visualDebugger)
	{
		create_debug_page(sceneNotebook);
	}

	if (m_manager->AddPane(renderingNotebook, wxLEFT, _("")) &&
	    m_manager->AddPane(sceneNotebook, wxRIGHT, _("")))
	{
		m_manager->GetPane(renderingNotebook).MinSize(wxSize(200, 250)).Name(FUSE_RENDERING_NOTEBOOK);
		m_manager->GetPane(sceneNotebook).MinSize(wxSize(200, 250)).Name(FUSE_SCENE_NOTEBOOK);

		sceneNotebook->Thaw();
		renderingNotebook->Thaw();

		m_manager->Update();

		m_scene->add_listener(this);
		m_scene->get_active_camera_node()->add_listener(this);

		m_initialized = true;
	}

	return m_initialized;
}

void wx_editor_gui::shutdown(void)
{
	m_scene->remove_listener(this);
}

bool wx_editor_gui::is_initialized(void) const
{
	return m_initialized;
}

void wx_editor_gui::create_menu(void)
{
	wxMenu * file = new wxMenu;
	file->Append(wxID_FILE, _("&Import"));
	file->Append(wxID_EXIT, _("E&xit"));

	file->Bind(wxEVT_COMMAND_MENU_SELECTED, [=](wxCommandEvent & event)
	{
		switch (event.GetId())
		{
		case wxID_FILE:
		{
			wxFileDialog openFileDialog(
				m_frame,
				_("Import scene file"), "", "", "FBX files (*.fbx)|*.fbx|All files|*",
				wxFD_OPEN | wxFD_FILE_MUST_EXIST);

			if (openFileDialog.ShowModal() != wxID_CANCEL)
			{
				if (!renderer_application::import_scene(openFileDialog.GetPath().c_str()))
				{
					renderer_application::signal_error(FUSE_LITERAL("Failed to import the selected scene."));
				}
			}
			break;
		}
			
		case wxID_EXIT:
			renderer_application::quit();
			break;
		}
	});
	
	m_menuBar = new wxMenuBar();
	m_menuBar->Append(file, _("&File"));

	m_frame->SetMenuBar(m_menuBar);
}

void wx_editor_gui::create_shadow_mapping_page(wxAuiNotebook * notebook)
{
	const int padding = 5;

	wxWindow   * panel        = new wxWindow(notebook, wxID_ANY);
	wxNotebook * algoNotebook = new wxNotebook(panel, wxID_ANY);

	/* VSM */

	{
		wxWindow   * vsm   = new wxWindow(algoNotebook, wxID_ANY);
		wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

		wx_choice * precisionChoice = new wx_choice(
			[&](int choice)
		{
			m_rendererConfiguration->set_vsm_float_precision(16 * choice + 16);
		}, vsm, wxID_ANY, _("16 Bit"), _("32 Bit"));

		precisionChoice->SetSelection(m_rendererConfiguration->get_vsm_float_precision() / 16 - 1);

		sizer->Add(new wxStaticText(vsm, wxID_ANY, _("Floating Point Precision")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(precisionChoice, 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(vsm, wxID_ANY, _("Minimum Variance")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(vsm, float, vsm_min_variance, 0, 1, .001f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(vsm, wxID_ANY, _("Minimum Bleeding")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(vsm, float, vsm_min_bleeding, 0, 1, .01f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(vsm, wxID_ANY, _("Blur Kernel Size")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(vsm, int, vsm_blur_kernel_size, 3, 25, 2), 0, wxEXPAND | wxALL, padding);

		vsm->SetSizerAndFit(sizer);

		algoNotebook->AddPage(vsm, _("VSM"));
	}

	/* EVSM2 */

	{
		wxWindow   * evsm2 = new wxWindow(algoNotebook, wxID_ANY);
		wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

		wx_choice * precisionChoice = new wx_choice(
			[&](int choice)
		{
			m_rendererConfiguration->set_vsm_float_precision(16 * choice + 16);
		}, evsm2, wxID_ANY, _("16 Bit"), _("32 Bit"));

		precisionChoice->SetSelection(m_rendererConfiguration->get_evsm2_float_precision() / 16 - 1);

		sizer->Add(new wxStaticText(evsm2, wxID_ANY, _("Floating Point Precision")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(precisionChoice, 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm2, wxID_ANY, _("Exponent")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm2, float, evsm2_exponent, 1, 50, .1f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm2, wxID_ANY, _("Minimum Variance")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm2, float, evsm2_min_variance, 0, 1, .001f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm2, wxID_ANY, _("Minimum Bleeding")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm2, float, evsm2_min_bleeding, 0, 1, .01f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm2, wxID_ANY, _("Blur Kernel Size")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm2, int, evsm2_blur_kernel_size, 3, 25, 2), 0, wxEXPAND | wxALL, padding);

		evsm2->SetSizerAndFit(sizer);

		algoNotebook->AddPage(evsm2, _("EVSM2"));
	}

	/* EVSM4 */

	{
		wxWindow   * evsm4 = new wxWindow(algoNotebook, wxID_ANY);
		wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

		wx_choice * precisionChoice = new wx_choice(
			[&](int choice)
		{
			m_rendererConfiguration->set_vsm_float_precision(16 * choice + 16);
		}, evsm4, wxID_ANY, _("16 Bit"), _("32 Bit"));

		precisionChoice->SetSelection(m_rendererConfiguration->get_evsm4_float_precision() / 16 - 1);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Floating Point Precision")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(precisionChoice, 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Negative Exponent")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm4, float, evsm4_negative_exponent, 1, 50, .1f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Positive Exponent")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm4, float, evsm4_positive_exponent, 1, 50, .1f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Minimum Variance")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm4, float, evsm4_min_variance, 0, 1, .001f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Minimum Bleeding")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm4, float, evsm4_min_bleeding, 0, 1, .01f), 0, wxEXPAND | wxALL, padding);

		sizer->Add(new wxStaticText(evsm4, wxID_ANY, _("Blur Kernel Size")), 0, wxEXPAND | wxALL, padding);
		sizer->Add(FUSE_WX_SPIN_RV(evsm4, int, evsm4_blur_kernel_size, 3, 25, 2), 0, wxEXPAND | wxALL, padding);

		evsm4->SetSizerAndFit(sizer);

		algoNotebook->AddPage(evsm4, _("EVSM4"));
	}

	/* Common */

	wxBoxSizer * panelSizer = new wxBoxSizer(wxVERTICAL);

	wx_choice * algoChoice = new wx_choice(
		[&](int choice)
	{
		m_rendererConfiguration->set_shadow_mapping_algorithm(static_cast<shadow_mapping_algorithm>(choice));
	}, panel, wxID_ANY, _("Off"), _("VSM"), _("EVSM2"), _("EVSM4"));

	algoChoice->SetSelection(static_cast<int>(m_rendererConfiguration->get_shadow_mapping_algorithm()));

	panelSizer->Add(new wxStaticText(panel, wxID_ANY, _("Current Algorithm")), 0, wxEXPAND | wxALL, padding);
	panelSizer->Add(algoChoice, 0, wxEXPAND | wxALL, padding);

	panelSizer->Add(new wxStaticText(panel, wxID_ANY, _("Shadow Map Resolution")), 0, wxEXPAND | wxALL, padding);
	panelSizer->Add(FUSE_WX_SPIN_RV(panel, int, shadow_map_resolution, 256, 4096, 256), 0, wxEXPAND | wxALL, padding);

	panelSizer->Add(algoNotebook, 1, wxEXPAND | wxALL, padding);

	panel->SetSizerAndFit(panelSizer);

	notebook->AddPage(panel, _("Shadow Mapping"));
}

void wx_editor_gui::create_skylight_page(wxAuiNotebook * notebook)
{
	const int padding = 5;

	wxWindow   * skylight = new wxWindow(notebook, wxID_ANY);
	wxBoxSizer * sizer    = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer * skySizer     = new wxStaticBoxSizer(wxVERTICAL, skylight, _("Sky Appearance"));
	wxStaticBoxSizer * ambientSizer = new wxStaticBoxSizer(wxVERTICAL, skylight, _("Ambient Light"));

	skySizer->Add(new wxStaticText(skySizer->GetStaticBox(), wxID_ANY, _("Zenith")), 0, wxEXPAND | wxALL, padding);
	skySizer->Add(FUSE_WX_SLIDER(skySizer->GetStaticBox(), float, m_scene->get_skydome()->get_zenith, m_scene->get_skydome()->set_zenith, 0, FUSE_HALF_PI), 0, wxEXPAND | wxALL, padding);

	skySizer->Add(new wxStaticText(skySizer->GetStaticBox(), wxID_ANY, _("Azimuth")), 0, wxEXPAND | wxALL, padding);
	skySizer->Add(FUSE_WX_SLIDER(skySizer->GetStaticBox(), float, m_scene->get_skydome()->get_azimuth, m_scene->get_skydome()->set_azimuth, -FUSE_PI, FUSE_PI), 0, wxEXPAND | wxALL, padding);

	skySizer->Add(new wxStaticText(skySizer->GetStaticBox(), wxID_ANY, _("Turbidity")), 0, wxEXPAND | wxALL, padding);
	skySizer->Add(FUSE_WX_SLIDER(skySizer->GetStaticBox(), float, m_scene->get_skydome()->get_turbidity, m_scene->get_skydome()->set_turbidity, 1, 10), 0, wxEXPAND | wxALL, padding);

	sizer->Add(skySizer, 0, wxEXPAND | wxALL, padding);

	ambientSizer->Add(new wxStaticText(ambientSizer->GetStaticBox(), wxID_ANY, _("Color")), 0, wxEXPAND | wxALL, padding);
	ambientSizer->Add(new wx_color_picker([&](const color_rgb & color) { m_scene->get_skydome()->set_ambient_color(color); }, ambientSizer->GetStaticBox(), wxID_ANY, m_scene->get_skydome()->get_ambient_color()), 0, wxEXPAND | wxALL, padding);

	ambientSizer->Add(new wxStaticText(ambientSizer->GetStaticBox(), wxID_ANY, _("Intensity")), 0, wxEXPAND | wxALL, padding);
	ambientSizer->Add(FUSE_WX_SPIN(ambientSizer->GetStaticBox(), float, m_scene->get_skydome()->get_ambient_intensity, m_scene->get_skydome()->set_ambient_intensity, 0, 100, 0.1f), 0, wxEXPAND | wxALL, padding);

	sizer->Add(ambientSizer, 0, wxEXPAND | wxALL, padding);

	skylight->SetSizerAndFit(sizer);

	notebook->AddPage(skylight, _("Skylight"));
}

void wx_editor_gui::create_debug_page(wxAuiNotebook * notebook)
{
	const int padding = 5;

	wxWindow   * debug = new wxWindow(notebook, wxID_ANY);
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

	{
		wxStaticBoxSizer * drawSizer = new wxStaticBoxSizer(wxVERTICAL, debug, _("Draw"));

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_selected_node_bounding_volume(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Selected Node Bounding Volume"), m_visualDebugger->get_draw_selected_node_bounding_volume()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_bounding_volumes(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Bounding Volumes"), m_visualDebugger->get_draw_bounding_volumes()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_octree(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Octree"), m_visualDebugger->get_draw_octree()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox(
			[&](bool value) {
				if (value) m_visualDebugger->add_persistent(FUSE_LITERAL("camera"), m_scene->get_active_camera_node()->get_camera()->get_frustum(), color_rgba(0, 1, 0, 1));
				else m_visualDebugger->remove_persistent(FUSE_LITERAL("camera"));
			},
			drawSizer->GetStaticBox(), wxID_ANY, _("Current Camera Frustum"), m_visualDebugger->get_draw_bounding_volumes()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_skydome(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Skydome"), m_visualDebugger->get_draw_skydome()), 0, wxEXPAND | wxALL, padding);

		sizer->Add(drawSizer, 0, wxEXPAND | wxALL, padding);
	}

	debug->SetSizerAndFit(sizer);

	notebook->AddPage(debug, _("Debug"));
}

void wx_editor_gui::create_camera_page(wxAuiNotebook * notebook)
{
	const int padding = 5;

	m_camera = new wxWindow(notebook, wxID_ANY);
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

	/*sizer->Add(new wxStaticText(camera, wxID_ANY, _("Field of View")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera_node()->get_fovx_deg, m_scene->get_active_camera_node()->set_fovx_deg, 15.f, 120.f, 5.f), 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(camera, wxID_ANY, _("Near Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera_node()->get_znear, m_scene->get_active_camera_node()->set_znear, .00001f, 100000.f, .1f), 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(camera, wxID_ANY, _("Far Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera_node()->get_zfar, m_scene->get_active_camera_node()->set_zfar, .00001f, 100000.f, .1f), 0, wxEXPAND | wxALL, padding);*/

	auto fov = new wx_spin<float>(
		[&](void)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		return c ? c->get_fovy_deg() : 0.f;
	},
		[&](float x)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		if (c)
		{
			c->set_fovy_deg(x);
		}
	},
		m_camera, wxID_ANY, 15.f, 120.f, 5.f);

	auto zfar = new wx_spin<float>(
		[&](void)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		return c ? c->get_zfar() : 0.f;
	},
		[&](float x)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		if (c)
		{
			c->set_zfar(x);
		}
	},
		m_camera, wxID_ANY, .00001f, 100000.f, .1f);

	auto znear = new wx_spin<float>(
		[&](void)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		return c ? c->get_znear() : 0.f;
	},
		[&](float x)
	{
		camera * c = m_scene->get_active_camera_node()->get_camera();
		if (c)
		{
			c->set_znear(x);
		}
	},
		m_camera, wxID_ANY, .00001f, 100000.f, .1f);

	sizer->Add(new wxStaticText(m_camera, wxID_ANY, _("Field of View")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(fov, 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(m_camera, wxID_ANY, _("Near Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(znear, 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(m_camera, wxID_ANY, _("Far Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(zfar, 0, wxEXPAND | wxALL, padding);

	m_camera->SetSizerAndFit(sizer);

	m_camera->Bind(FUSE_EVENT_REFRESH, [=](wxCommandEvent & event)
	{
		fov->update_value();
		znear->update_value();
		zfar->update_value();
	});

	notebook->AddPage(m_camera, _("Camera"));
}

void wx_editor_gui::add_branch(scene_graph_node * root)
{
	std::deque<scene_graph_node*> stack = { root };

	do
	{
		scene_graph_node * node   = stack.front();
		scene_graph_node * parent = node->get_parent();

		string_t name = node->get_name();
		wxTreeItemId id;

		if (parent)
		{
			id = m_sgTree->AppendItem(
				m_nodeItemMap[parent],
				name.empty() ? _("<unnamed node>") : name.c_str());
		}
		else
		{
			id = m_sgTree->AddRoot(_("<root>"));
		}

		m_nodeItemMap[node] = id;
		m_itemNodeMap[id] = node;

		std::copy(node->begin(), node->end(), std::back_inserter(stack));
		stack.pop_front();
	} while (!stack.empty());
}

void wx_editor_gui::remove_branch(scene_graph_node * root)
{
	std::deque<scene_graph_node*> stack = { root };

	do
	{
		scene_graph_node * node   = stack.front();

		auto nodeItemIt = m_nodeItemMap.find(node);

		wxTreeItemId id = nodeItemIt->second;

		m_sgTree->Delete(id);

		m_nodeItemMap.erase(nodeItemIt);
		m_itemNodeMap.erase(m_itemNodeMap.find(id));

		std::copy(node->begin(), node->end(), std::back_inserter(stack));
		stack.pop_front();
	} while (!stack.empty());
}

void wx_editor_gui::create_scene_graph_page(wxAuiNotebook * notebook)
{
	const int padding = 5;

	wxWindow   * sg    = new wxWindow(notebook, wxID_ANY);
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer * localTranslationBox    = new wxStaticBoxSizer(wxVERTICAL, sg, _("Local translation"));
	wxBoxSizer       * localTranslationFields = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer * localScaleBox    = new wxStaticBoxSizer(wxVERTICAL, sg, _("Local scale"));
	wxBoxSizer       * localScaleFields = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBoxSizer * localRotationBox    = new wxStaticBoxSizer(wxVERTICAL, sg, _("Local rotation"));
	wxBoxSizer       * localRotationFields = new wxBoxSizer(wxHORIZONTAL);

	m_sgTree = new wxTreeCtrl(sg);

	update_scene_graph();
	set_selected_node(m_scene->get_scene_graph()->get_root());

	/* Local translation */

	auto localTranslationX = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_x(m_selectedNode->get_local_translation());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_translation();
			vec128_set_x(v, x);
			m_selectedNode->set_local_translation(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localTranslationY = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_y(m_selectedNode->get_local_translation());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_translation();
			vec128_set_y(v, x);
			m_selectedNode->set_local_translation(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localTranslationZ = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_z(m_selectedNode->get_local_translation());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_translation();
			vec128_set_z(v, x);
			m_selectedNode->set_local_translation(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	/* Local scale */

	auto localScaleX = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_x(m_selectedNode->get_local_scale());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_scale();
			vec128_set_x(v, x);
			m_selectedNode->set_local_scale(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localScaleY = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_y(m_selectedNode->get_local_scale());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_scale();
			vec128_set_y(v, x);
			return m_selectedNode->set_local_scale(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localScaleZ = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			return vec128_get_z(m_selectedNode->get_local_scale());
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			auto v = m_selectedNode->get_local_scale();
			vec128_set_z(v, x);
			return m_selectedNode->set_local_scale(v);
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	/* Local rotation */

	auto localRotationX = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
			float3 e = to_euler(q);
			return to_degrees(e.x);
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			if (m_selectedNode)
			{
				quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
				float3 e = to_euler(q);
				e.x = to_radians(x);
				m_selectedNode->set_local_rotation(to_quaternion(e));
			}
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localRotationY = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
			float3 e = to_euler(q);
			return to_degrees(e.y);
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
			float3 e = to_euler(q);
			e.y = to_radians(x);
			m_selectedNode->set_local_rotation(to_quaternion(e));
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	auto localRotationZ = new wx_spin<float>(
		[&]()
	{
		if (m_selectedNode)
		{
			quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
			float3 e = to_euler(q);
			return to_degrees(e.z);
		}
		return 0.f;
	},
		[&](float x)
	{
		if (m_selectedNode)
		{
			quaternion q = to_quaternion(m_selectedNode->get_local_rotation());
			float3 e = to_euler(q);
			e.z = to_radians(x);
			m_selectedNode->set_local_rotation(to_quaternion(e));
		}
	}, sg, wxID_ANY, -FLT_MAX, FLT_MAX, .1f);

	m_sgTree->Bind(wxEVT_TREE_BEGIN_DRAG,
		[&](wxTreeEvent & event)
	{
		wxTreeItemId item = event.GetItem();

		if (item != m_sgTree->GetRootItem())
		{
			m_draggedItem = item;
			event.Allow();
		}
	});

	m_sgTree->Bind(wxEVT_TREE_END_DRAG,
		[&](wxTreeEvent & event)
	{
		wxTreeItemId parent = event.GetItem();
		wxTreeItemId item = m_draggedItem;

		auto parentIt = m_itemNodeMap.find(parent);
		auto nodeIt   = m_itemNodeMap.find(item);

		if (parentIt != m_itemNodeMap.end() &&
		    nodeIt != m_itemNodeMap.end())
		{
			scene_graph_node * parent = parentIt->second;
			scene_graph_node * node   = nodeIt->second;

			if (parent->is_ancestor(node))
			{
				renderer_application::signal_error(FUSE_LITERAL("No loops allowed in the scene graph, cannot attach an ancestor to a descendant."));
			}
			else if (parent != node)
			{
				remove_branch(node);
				node->set_parent(parent);
				add_branch(node);
			}
		}
	});

	/* Size settings */

	localTranslationX->SetMinSize(wxSize(64, 24));
	localTranslationY->SetMinSize(wxSize(64, 24));
	localTranslationZ->SetMinSize(wxSize(64, 24));

	localScaleX->SetMinSize(wxSize(64, 24));
	localScaleY->SetMinSize(wxSize(64, 24));
	localScaleZ->SetMinSize(wxSize(64, 24));

	localRotationX->SetMinSize(wxSize(64, 24));
	localRotationY->SetMinSize(wxSize(64, 24));
	localRotationZ->SetMinSize(wxSize(64, 24));

	/* Add elements */

	//localTranslation->Add(new wxStaticText(sg, wxID_ANY, _("Translation")), 0, wxEXPAND | wxALL, padding);
	localTranslationFields->Add(localTranslationX, 1, wxEXPAND | wxALL);
	localTranslationFields->Add(localTranslationY, 1, wxEXPAND | wxALL);
	localTranslationFields->Add(localTranslationZ, 1, wxEXPAND | wxALL);

	localScaleFields->Add(localScaleX, 1, wxEXPAND | wxALL);
	localScaleFields->Add(localScaleY, 1, wxEXPAND | wxALL);
	localScaleFields->Add(localScaleZ, 1, wxEXPAND | wxALL);

	localRotationFields->Add(localRotationX, 1, wxEXPAND | wxALL);
	localRotationFields->Add(localRotationY, 1, wxEXPAND | wxALL);
	localRotationFields->Add(localRotationZ, 1, wxEXPAND | wxALL);

	localTranslationBox->Add(localTranslationFields, 1, wxEXPAND | wxALL);
	localScaleBox->Add(localScaleFields, 1, wxEXPAND | wxALL);
	localRotationBox->Add(localRotationFields, 1, wxEXPAND | wxALL);

	sizer->Add(m_sgTree, 1, wxEXPAND | wxALL, padding);

	sizer->Add(localTranslationBox, 0, wxEXPAND | wxALL, padding);
	sizer->Add(localScaleBox, 0, wxEXPAND | wxALL, padding);
	sizer->Add(localRotationBox, 0, wxEXPAND | wxALL, padding);

	sg->SetSizerAndFit(sizer);

	notebook->AddPage(sg, _("Scene Graph"));

	m_sgTree->Bind(wxEVT_TREE_SEL_CHANGED,
		[&]
	(wxTreeEvent & event)
	{
		wxTreeItemId item = event.GetItem();
		m_selectedNode = m_itemNodeMap[item];
		update_selected_object();
	});

	m_sgTree->Bind(FUSE_EVENT_REFRESH,
		[=](wxCommandEvent & event)
	{
		localTranslationX->update_value();
		localTranslationY->update_value();
		localTranslationZ->update_value();
		localScaleX->update_value();
		localScaleY->update_value();
		localScaleZ->update_value();
		localRotationX->update_value();
		localRotationY->update_value();
		localRotationZ->update_value();
	});
}

void wx_editor_gui::update_scene_graph(void)
{
	m_sgTree->DeleteAllItems();
	add_branch(m_scene->get_scene_graph()->get_root());}

void wx_editor_gui::update_camera(void)
{
	wxPostEvent(m_camera, wxCommandEvent(FUSE_EVENT_REFRESH));
}

void wx_editor_gui::update_selected_object(void)
{
	wxPostEvent(m_sgTree, wxCommandEvent(FUSE_EVENT_REFRESH));
}

scene_graph_node * wx_editor_gui::get_selected_node(void) const
{
	return m_selectedNode;
}

void wx_editor_gui::set_selected_node(scene_graph_node * node)
{
	auto it = m_nodeItemMap.find(node);

	if (it != m_nodeItemMap.end())
	{
		if (m_selectedNode)
		{
			m_selectedNode->remove_listener(this);
		}

		m_sgTree->SelectItem(it->second);

		if (node)
		{
			node->remove_listener(this);
		}
	}
}

void wx_editor_gui::on_scene_graph_node_destruction(scene_graph_node * node)
{
	//update_scene_graph();
}

void wx_editor_gui::on_scene_graph_node_move(scene_graph_node * node, const mat128 & oldTransform, const mat128 & newTransform)
{
	if (node == m_scene->get_active_camera_node())
	{
		update_camera();
	}
	if (node == m_selectedNode)
	{
		update_selected_object();
	}
}

void wx_editor_gui::on_scene_active_camera_change(scene * scene, scene_graph_camera * oldCamera, scene_graph_camera * newCamera)
{
	if (oldCamera)
	{
		oldCamera->remove_listener(this);
	}
	if (newCamera)
	{
		newCamera->add_listener(this);
		update_camera();
	}
}

void wx_editor_gui::on_scene_clear(scene * scene)
{

}

#endif