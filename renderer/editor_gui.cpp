#ifdef FUSE_USE_EDITOR_GUI

#include "editor_gui.hpp"

#include <fuse/rocket_input.hpp>
#include <algorithm>

#include <Rocket/Debugger.h>
#include <Rocket/Controls.h>

#include <cctype>

using namespace fuse;
using namespace fuse::gui;

editor_gui::editor_gui(void)
{
	m_context = nullptr;
}

editor_gui::~editor_gui(void)
{
	shutdown();
}

bool editor_gui::init(scene * scene, renderer_configuration * r)
{

	m_context = Rocket::Core::CreateContext("editor_gui", Rocket::Core::Vector2i(256, 256));

	Rocket::Debugger::Initialise(m_context);

	return m_objectPanel.init(m_context) &&
	       m_renderOptions.init(m_context, r) &&
	       m_lightPanel.init(m_context, scene);
}

void editor_gui::shutdown(void)
{
	
	if (m_context)
	{

		m_objectPanel.shutdown();
		m_renderOptions.shutdown();
		m_lightPanel.shutdown();

		m_context->RemoveReference();

		m_context = nullptr;

	}

}

void editor_gui::on_resize(UINT width, UINT height)
{
	assert(m_context && "Can't call editor_gui::on_resize before initialization.");
	m_context->SetDimensions(Rocket::Core::Vector2i(width, height));
}

void editor_gui::render(void)
{
	m_context->Render();
}

void editor_gui::update(void)
{
	m_context->Update();
}

void editor_gui::select_object(renderable * object)
{
	m_objectPanel.set_object(object);
}

LRESULT editor_gui::on_keyboard(WPARAM wParam, LPARAM lParam)
{
	return rocket_keyboard_input_translate(m_context, wParam, lParam) ? 0 : 1;
}

LRESULT editor_gui::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	if (uMsg == WM_CHAR)
	{
		if (isprint(wParam))
		{
			m_context->ProcessTextInput(wParam);
		}
	}
	/*else if (uMsg == WM_KEYDOWN)
	{
		rocket_keyboard_input_translate(m_context, wParam, lParam);
	}*/

	return 0;

}

LRESULT editor_gui::on_mouse(WPARAM wParam, LPARAM lParam)
{

	if (wParam == WM_MOUSEMOVE)
	{

		LPMOUSEHOOKSTRUCT mouseStruct = (LPMOUSEHOOKSTRUCT) lParam;

		POINT pt = mouseStruct->pt;
		ScreenToClient(mouseStruct->hwnd, &pt);

		m_context->ProcessMouseMove(pt.x, pt.y, 0);

	}
	else
	{

		int vk = -1;
		bool pressed;

		switch (wParam)
		{
		case WM_RBUTTONDOWN:
			vk = 1;
			pressed = true;
			break;
		case WM_RBUTTONUP:
			vk = 1;
			pressed = false;
			break;
		case WM_LBUTTONDOWN:
			vk = 0;
			pressed = true;
			break;
		case WM_LBUTTONUP:
			vk = 0;
			pressed = false;
			break;
		case WM_MBUTTONDOWN:
			vk = 2;
			pressed = true;
			break;
		case WM_MBUTTONUP:
			vk = 2;
			pressed = false;
			break;
		}


		if (vk >= 0)
		{

			if (pressed)
			{
				m_context->ProcessMouseButtonDown(vk, 0);
			}
			else
			{
				m_context->ProcessMouseButtonUp(vk, 0);
			}

		}

	}

	return 0;
}

void editor_gui::set_debugger_visibility(bool visibility)
{
	Rocket::Debugger::SetVisible(visibility);
}

bool editor_gui::get_debugger_visibility(void) const
{
	return Rocket::Debugger::IsVisible();
}

void editor_gui::set_render_options_visibility(bool visibility)
{
	if (visibility)
	{
		m_renderOptions.show();
	}
	else
	{
		m_renderOptions.hide();
	}
}

bool editor_gui::get_render_options_visibility(void) const
{
	return m_renderOptions.is_visible();
}

void editor_gui::set_object_panel_visibility(bool visibility)
{
	if (visibility)
	{
		m_objectPanel.show();
	}
	else
	{
		m_objectPanel.hide();
	}
}

bool editor_gui::get_object_panel_visibility(void) const
{
	return m_objectPanel.is_visible();
}


void editor_gui::set_light_panel_visibility(bool visibility)
{
	if (visibility)
	{
		m_lightPanel.show();
	}
	else
	{
		m_lightPanel.hide();
	}
}

bool editor_gui::get_light_panel_visibility(void) const
{
	return m_lightPanel.is_visible();
}

/* gui_panel */

gui_panel::gui_panel(void) : m_panel(nullptr) { }

gui_panel::~gui_panel(void) { shutdown(); }

void gui_panel::shutdown(void)
{
	if (m_panel)
	{
		m_panel->Close();
		m_panel = nullptr;
	}
}

void gui_panel::show(int flags)
{
	m_panel->Show(flags);
}

void gui_panel::hide(void)
{
	m_panel->Hide();
}

bool gui_panel::is_visible(void) const
{
	return m_panel->IsVisible();
}

/* object_panel */

bool object_panel::init(Rocket::Core::Context * context)
{

	m_configuration = nullptr;

	m_panel = context->LoadDocument("ui/selected_object.rml");

	if (m_panel)
	{

		m_panel->AddEventListener("click", this);
		m_panel->AddEventListener("change", this);

		m_panel->GetElementById("title")->SetInnerRML(m_panel->GetTitle());

		return true;
	}
	
	return false;
}

void object_panel::set_object(renderable * object)
{
	
	m_configuration = object;

	if (m_configuration)
	{
		show();
	}
	else
	{
		hide();
	}
	
}

void object_panel::ProcessEvent(Rocket::Core::Event & event)
{

	auto element = event.GetTargetElement();

	if (event.GetType() == "click")
	{
	}
	else if (event.GetType() == "change")
	{

		auto attribute = element->GetAttribute("type");

		if (attribute->Get<Rocket::Core::String>() == "range")
		{
			Rocket::Controls::ElementFormControlInput * slider = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(element);
			//element->SetInnerRML("<span class=\"range_value\">" + slider->GetValue() + "</span>");
		}
	}

}

/* color_panel */

bool color_panel::init(Rocket::Core::Context * context)
{

	m_panel = context->LoadDocument("ui/color_panel.rml");

	if (m_panel)
	{

		m_panel->AddEventListener("click", this);
		m_panel->AddEventListener("change", this);

		m_panel->GetElementById("title")->SetInnerRML(m_panel->GetTitle());

		fill();

		return true;

	}

	return false;

}

void color_panel::fill(void)
{

	m_filling = true;

	auto red   = m_panel->GetElementById("color_picker_red");
	auto green = m_panel->GetElementById("color_picker_green");
	auto blue  = m_panel->GetElementById("color_picker_blue");

	Rocket::Core::String redStr, greenStr, blueStr;

	Rocket::Core::TypeConverter<Rocket::Core::byte, Rocket::Core::String>::Convert(m_color.red, redStr);
	Rocket::Core::TypeConverter<Rocket::Core::byte, Rocket::Core::String>::Convert(m_color.green, greenStr);
	Rocket::Core::TypeConverter<Rocket::Core::byte, Rocket::Core::String>::Convert(m_color.blue, blueStr);

	dynamic_cast<Rocket::Controls::ElementFormControlInput*>(red)->SetValue(redStr);
	dynamic_cast<Rocket::Controls::ElementFormControlInput*>(green)->SetValue(greenStr);
	dynamic_cast<Rocket::Controls::ElementFormControlInput*>(blue)->SetValue(blueStr);

	auto preview = m_panel->GetElementById("color_picker_preview");
	preview->SetProperty("background-color", Rocket::Core::Property(m_color, Rocket::Core::Property::COLOUR));

	m_filling = false;

}

void color_panel::ProcessEvent(Rocket::Core::Event & event)
{

	if (m_filling) { return; }

	auto element = event.GetTargetElement();

	if (event.GetType() == "change")
	{

		auto attribute = element->GetAttribute("type");

		if (element->GetId() == "color_picker_red" ||
			element->GetId() == "color_picker_green" ||
			element->GetId() == "color_picker_blue")
		{

			auto red   = m_panel->GetElementById("color_picker_red");
			auto green = m_panel->GetElementById("color_picker_green");
			auto blue  = m_panel->GetElementById("color_picker_blue");

			auto redStr   = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(red)->GetValue();
			auto greenStr = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(green)->GetValue();
			auto blueStr  = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(blue)->GetValue();

			int redChannel, greenChannel, blueChannel;

			Rocket::Core::TypeConverter<Rocket::Core::String, int>::Convert(redStr, redChannel);
			Rocket::Core::TypeConverter<Rocket::Core::String, int>::Convert(greenStr, greenChannel);
			Rocket::Core::TypeConverter<Rocket::Core::String, int>::Convert(blueStr, blueChannel);

			auto preview = m_panel->GetElementById("color_picker_preview");
			Rocket::Core::Colourb color(redChannel, greenChannel, blueChannel, 0xFF);

			preview->SetProperty("background-color", Rocket::Core::Property(color, Rocket::Core::Property::COLOUR));

		}

	}
	else if (event.GetType() == "click")
	{
		if (element->GetId() == "color_picker_submit")
		{
			auto preview = m_panel->GetElementById("color_picker_preview");
			m_color = preview->GetProperty("background-color")->Get<Rocket::Core::Colourb>();
			shutdown();
		}
		else if (element->GetId() == "color_picker_cancel")
		{
			shutdown();
		}
	}

}

/* light_panel */

bool light_panel::init(Rocket::Core::Context * context, scene * scene)
{

	m_scene = scene;

	m_panel = context->LoadDocument("ui/lights.rml");
	m_light = nullptr;

	if (m_panel)
	{

		m_panel->AddEventListener("click", this);
		m_panel->AddEventListener("change", this);

		m_panel->GetElementById("title")->SetInnerRML(m_panel->GetTitle());

		fill_form();

		return true;

	}

	return false;
}

void light_panel::shutdown(void)
{
	m_colorPanel.shutdown();
	gui_panel::shutdown();
}

void light_panel::set_light(light * light)
{

	m_light = light;

	if (m_light)
	{
		fill_form();
		show();
	}

}

static void direction_to_elevation_azimuth(const XMFLOAT3 & direction, float & elevation, float & azimuth)
{
	elevation = XMConvertToDegrees(std::asin(direction.y));
	azimuth = XMConvertToDegrees(std::atan(direction.x / direction.y)) - 90.f;
}

void elevation_azimuth_to_direction(float elevation, float azimuth, XMFLOAT3 & direction)
{

	float sinAzimuth, cosAzimuth;
	float sinElevation, cosElevation;

	XMScalarSinCos(&sinAzimuth, &cosAzimuth, XMConvertToRadians(azimuth));
	XMScalarSinCos(&sinElevation, &cosElevation, XMConvertToRadians(elevation));

	direction.y = sinElevation;
	direction.x = cosElevation * cosAzimuth;
	direction.z = cosElevation * sinAzimuth;

}

void light_panel::ProcessEvent(Rocket::Core::Event & event)
{

	auto element = event.GetTargetElement();
	auto document = element->GetOwnerDocument();

	if (event.GetType() == "unload" && document == m_colorPanel.get_panel())
	{

		Rocket::Core::Element * element;

		switch (m_light->type)
		{
		case FUSE_LIGHT_TYPE_DIRECTIONAL:
			element = m_panel->GetElementById("directional_color");
			break;
		default:
			return;
		}

		auto color = m_colorPanel.get_color();
		element->SetProperty("background-color", Rocket::Core::Property(color, Rocket::Core::Property::COLOUR));

		m_light->color = color_rgb(color.red, color.green, color.blue) / 255.f;

		
	}
	else if (event.GetType() == "click")
	{
		if (element->GetId() == "next")
		{
			next_light();
		}
		else if (element->GetId() == "directional_color")
		{
			if (m_colorPanel.init(m_panel->GetContext()))
			{
				m_colorPanel.show(Rocket::Core::ElementDocument::MODAL);
				m_colorPanel.get_panel()->AddEventListener("unload", this);
			}
		}
	}
	else if (event.GetType() == "change")
	{

		if (m_filling)
		{
			return;
		}

		if (element->GetId() == "directional_elevation" || element->GetId() == "directional_azimuth")
		{

			float elevation, azimuth;
			
			auto elevationText = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById("directional_elevation"))->GetValue();
			auto azimuthText = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById("directional_azimuth"))->GetValue();

			if (Rocket::Core::TypeConverter<Rocket::Core::String, float>::Convert(elevationText, elevation) &&
				Rocket::Core::TypeConverter<Rocket::Core::String, float>::Convert(azimuthText, azimuth))
			{
				elevation_azimuth_to_direction(elevation, azimuth, m_light->direction);
			}

		}
		else if (element->GetId() == "directional_intensity")
		{

			float intensity;

			auto intensityStr = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(element)->GetValue();

			if (Rocket::Core::TypeConverter<Rocket::Core::String, float>::Convert(intensityStr, intensity))
			{
				m_light->intensity = intensity;
			}
		}
		
	}

}

void light_panel::fill_form(void)
{

	m_filling = true;

	using namespace Rocket::Core;

	String lightType;

	if (m_light)
	{
		switch (m_light->type)
		{
		case FUSE_LIGHT_TYPE_DIRECTIONAL:

			lightType = "directional";
			{

				float elevation, azimuth;

				direction_to_elevation_azimuth(m_light->direction, elevation, azimuth);
				
				{

					String value;

					TypeConverter<float, String>::Convert(elevation, value);
					dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById("directional_elevation"))->SetValue(value);

				}

				{

					String value;

					TypeConverter<float, String>::Convert(azimuth, value);
					dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById("directional_azimuth"))->SetValue(value);

				}

				{

					String value;

					TypeConverter<float, String>::Convert(m_light->intensity, value);
					dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById("directional_intensity"))->SetValue(value);

				}

				{

					Rocket::Core::String redStr, greenStr, blueStr;
					Rocket::Core::Colourb color;
					
					color.red   = m_light->color.r * 255.f;
					color.green = m_light->color.g * 255.f;
					color.blue  = m_light->color.b * 255.f;

					Rocket::Core::Element * colorElement;

					switch (m_light->type)
					{
					case FUSE_LIGHT_TYPE_DIRECTIONAL:
						colorElement = m_panel->GetElementById("directional_color");
						break;
					default:
						return;
					}

					colorElement->SetProperty("background-color", Rocket::Core::Property(color, Rocket::Core::Property::COLOUR));
					m_colorPanel.set_color(color);

				}

			}

			break;
		case FUSE_LIGHT_TYPE_SPOTLIGHT:
			lightType = "spotlight";
			break;
		case FUSE_LIGHT_TYPE_POINTLIGHT:
			lightType = "pointlight";
			break;
		}
	}

	Rocket::Core::ElementList elements;

	m_panel->GetElementsByClassName(elements, "options");

	for (auto e : elements)
	{
		if (e->GetId() != lightType &&
			e->GetId() != "common")
		{
			e->SetProperty("visibility", "hidden");
		}
		else
		{
			e->SetProperty("visibility", "visible");
		}
	}

	m_filling = false;

}

void light_panel::next_light(void)
{

	auto lights = m_scene->get_lights();

	if (lights.first == lights.second)
	{
		return;
	}

	auto it = std::find(lights.first, lights.second, m_light);

	it = (it == lights.second ? it : std::next(it));

	if (it == lights.second)
	{
		it = lights.first;
	}

	m_light = *it;

	fill_form();

}

/* render_options */

bool render_options::init(Rocket::Core::Context * context, renderer_configuration * r)
{

	m_configuration = r;

	if (m_configuration)
	{

		m_panel = context->LoadDocument("ui/render_options.rml");

		if (m_panel)
		{

			m_panel->AddEventListener("click", this);
			m_panel->AddEventListener("change", this);

			m_panel->GetElementById("title")->SetInnerRML(m_panel->GetTitle());

			fill_form();

			return true;
		}

	}

	return false;

}

void render_options::fill_form(void)
{

	using namespace Rocket::Core;

	m_filling = true;

#define __RENDER_OPTIONS_FILL(Type, Property) \
	{\
		String value;\
		Type variable = m_configuration->get_ ## Property ();\
		TypeConverter<Type, String>::Convert(variable, value);\
		dynamic_cast<Rocket::Controls::ElementFormControlInput*>(m_panel->GetElementById(#Property))->SetValue(value);\
	}

	__RENDER_OPTIONS_FILL(float, shadow_map_resolution)
	__RENDER_OPTIONS_FILL(float, vsm_min_variance)
	__RENDER_OPTIONS_FILL(float, vsm_min_bleeding)
	__RENDER_OPTIONS_FILL(float, vsm_blur_kernel_size)

	__RENDER_OPTIONS_FILL(float, evsm2_exponent)
	__RENDER_OPTIONS_FILL(float, evsm2_min_variance)
	__RENDER_OPTIONS_FILL(float, evsm2_min_bleeding)
	__RENDER_OPTIONS_FILL(float, evsm2_blur_kernel_size)

	__RENDER_OPTIONS_FILL(float, evsm4_positive_exponent)
	__RENDER_OPTIONS_FILL(float, evsm4_negative_exponent)
	__RENDER_OPTIONS_FILL(float, evsm4_min_variance)
	__RENDER_OPTIONS_FILL(float, evsm4_min_bleeding)
	__RENDER_OPTIONS_FILL(float, evsm4_blur_kernel_size)

	dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(m_panel->GetElementById("shadow_mapping_algorithm"))->
		SetSelection(static_cast<int>(m_configuration->get_shadow_mapping_algorithm()));

	dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(m_panel->GetElementById("vsm_float_precision"))->
		SetSelection(static_cast<int>(std::log2(m_configuration->get_vsm_float_precision()) / 4 - 1));

	dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(m_panel->GetElementById("evsm2_float_precision"))->
		SetSelection(static_cast<int>(std::log2(m_configuration->get_evsm2_float_precision()) / 4 - 1));

	dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(m_panel->GetElementById("evsm4_float_precision"))->
		SetSelection(static_cast<int>(std::log2(m_configuration->get_evsm4_float_precision()) / 4 - 1));

	m_filling = false;

}

void render_options::ProcessEvent(Rocket::Core::Event & event)
{

	if (m_filling) { return; }

#define __RENDER_OPTIONS_CHANGE(Type, Property) \
	else if (element->GetId() == #Property)\
	{\
		float value;\
		auto text = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(element)->GetValue();\
		if (Rocket::Core::TypeConverter<Rocket::Core::String, Type>::Convert(text, value))\
		{\
			m_configuration->set_ ## Property (value);\
		}\
	}	

	auto element = event.GetTargetElement();
	auto document = element->GetOwnerDocument();

	if (event.GetType() == "change")
	{

		if (element->GetId() == "shadow_mapping_algorithm")
		{
			int selection = dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(element)->GetSelection();
			m_configuration->set_shadow_mapping_algorithm(static_cast<shadow_mapping_algorithm>(selection));
		}
		else if (element->GetId() == "vsm_float_precision")
		{
			int selection = dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(element)->GetSelection();
			m_configuration->set_vsm_float_precision(16u << selection);
		}
		else if (element->GetId() == "evsm2_float_precision")
		{
			int selection = dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(element)->GetSelection();
			m_configuration->set_evsm2_float_precision(16u << selection);
		}
		else if (element->GetId() == "evsm4_float_precision")
		{
			int selection = dynamic_cast<Rocket::Controls::ElementFormControlSelect *>(element)->GetSelection();
			m_configuration->set_evsm4_float_precision(16u << selection);
		}

		__RENDER_OPTIONS_CHANGE(float, shadow_map_resolution)

		__RENDER_OPTIONS_CHANGE(float, vsm_min_variance)
		__RENDER_OPTIONS_CHANGE(float, vsm_min_bleeding)
		__RENDER_OPTIONS_CHANGE(float, vsm_blur_kernel_size)

		__RENDER_OPTIONS_CHANGE(float, evsm2_exponent)
		__RENDER_OPTIONS_CHANGE(float, evsm2_min_variance)
		__RENDER_OPTIONS_CHANGE(float, evsm2_min_bleeding)
		__RENDER_OPTIONS_CHANGE(float, evsm2_blur_kernel_size)

		__RENDER_OPTIONS_CHANGE(float, evsm4_positive_exponent)
		__RENDER_OPTIONS_CHANGE(float, evsm4_negative_exponent)
		__RENDER_OPTIONS_CHANGE(float, evsm4_min_variance)
		__RENDER_OPTIONS_CHANGE(float, evsm4_min_bleeding)
		__RENDER_OPTIONS_CHANGE(float, evsm4_blur_kernel_size)
		
	}

}


#endif