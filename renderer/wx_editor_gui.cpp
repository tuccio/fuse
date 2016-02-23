#ifdef FUSE_WXWIDGETS

#define FUSE_WX_SLIDER(Parent, Type, Get, Set, Min, Max) new wx_slider<Type>([&](Type x) { Set(x); }, Parent, wxID_ANY, Get(), Min, Max)
#define FUSE_WX_SPIN(Parent, Type, Get, Set, Min, Max, Step) new wx_spin<Type>([&](Type x) { Set(x); }, Parent, wxID_ANY, Get(), Min, Max, Step)

#define FUSE_WX_SLIDER_RV(Parent, Type, Variable, Min, Max) FUSE_WX_SLIDER(Parent, Type, m_rendererConfiguration->get_ ## Variable, m_rendererConfiguration->set_ ## Variable, Min, Max)
#define FUSE_WX_SPIN_RV(Parent, Type, Variable, Min, Max, Step) FUSE_WX_SPIN(Parent, Type, m_rendererConfiguration->get_ ## Variable, m_rendererConfiguration->set_ ## Variable, Min, Max, Step)

#include "wx_editor_gui.hpp"

#include <fuse/wx_helpers.hpp>

using namespace fuse;

bool wx_editor_gui::init(wxWindow * window, scene * scene, renderer_configuration * r, visual_debugger * visualDebugger)
{

	m_window  = window;
	m_manager = wxAuiManager::GetManager(window);
	m_scene   = scene;

	m_visualDebugger        = visualDebugger;
	m_rendererConfiguration = r;

	wxAuiNotebook * renderingNotebook = new wxAuiNotebook(window, wxID_ANY);
	wxAuiNotebook * sceneNotebook     = new wxAuiNotebook(window, wxID_ANY);

	renderingNotebook->Freeze();
	sceneNotebook->Freeze();

	create_shadow_mapping_page(renderingNotebook);
	create_skylight_page(sceneNotebook);
	create_camera_page(sceneNotebook);

	if (visualDebugger)
	{
		create_debug_page(sceneNotebook);
	}

	m_manager->AddPane(renderingNotebook, wxLEFT, _(""));
	m_manager->AddPane(sceneNotebook, wxRIGHT, _(""));

	m_manager->GetPane(renderingNotebook).MinSize(wxSize(200, 250)).Name("wx_editor_gui::rendering_notebook");
	m_manager->GetPane(sceneNotebook).MinSize(wxSize(200, 250)).Name("wx_editor_gui::scene_notebook");

	sceneNotebook->Thaw();
	renderingNotebook->Thaw();

	m_manager->Update();

	return true;

}

void wx_editor_gui::shutdown(void)
{

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
	skySizer->Add(FUSE_WX_SLIDER(skySizer->GetStaticBox(), float, m_scene->get_skydome()->get_zenith, m_scene->get_skydome()->set_zenith, 0, half_pi<float>()), 0, wxEXPAND | wxALL, padding);

	skySizer->Add(new wxStaticText(skySizer->GetStaticBox(), wxID_ANY, _("Azimuth")), 0, wxEXPAND | wxALL, padding);
	skySizer->Add(FUSE_WX_SLIDER(skySizer->GetStaticBox(), float, m_scene->get_skydome()->get_azimuth, m_scene->get_skydome()->set_azimuth, -pi<float>(), pi<float>()), 0, wxEXPAND | wxALL, padding);

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

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_bounding_volumes(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Bounding Volumes"), m_visualDebugger->get_draw_bounding_volumes()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox([&](bool value) { m_visualDebugger->set_draw_octree(value); },
			drawSizer->GetStaticBox(), wxID_ANY, _("Octree"), m_visualDebugger->get_draw_octree()), 0, wxEXPAND | wxALL, padding);

		drawSizer->Add(new wx_checkbox(
			[&](bool value) {
				if (value) m_visualDebugger->add_persistent("camera", m_scene->get_active_camera()->get_frustum(), color_rgba(0, 1, 0, 1));
				else m_visualDebugger->remove_persistent("camera");
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

	wxWindow   * camera = new wxWindow(notebook, wxID_ANY);
	wxBoxSizer * sizer  = new wxBoxSizer(wxVERTICAL);

	sizer->Add(new wxStaticText(camera, wxID_ANY, _("Field of View")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera()->get_fovx_deg, m_scene->get_active_camera()->set_fovx_deg, 15.f, 120.f, 5.f), 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(camera, wxID_ANY, _("Near Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera()->get_znear, m_scene->get_active_camera()->set_znear, .00001f, 100000.f, .1f), 0, wxEXPAND | wxALL, padding);

	sizer->Add(new wxStaticText(camera, wxID_ANY, _("Far Clip Distance")), 0, wxEXPAND | wxALL, padding);
	sizer->Add(FUSE_WX_SPIN(camera, float, m_scene->get_active_camera()->get_zfar, m_scene->get_active_camera()->set_zfar, .00001f, 100000.f, .1f), 0, wxEXPAND | wxALL, padding);

	camera->SetSizerAndFit(sizer);

	notebook->AddPage(camera, _("Camera"));

}

#endif