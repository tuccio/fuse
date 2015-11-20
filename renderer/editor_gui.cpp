#ifdef FUSE_USE_EDITOR_GUI

#include "editor_gui.hpp"

#include <fuse/rocket_input.hpp>
#include <algorithm>

#include <Rocket/Debugger.h>
#include <Rocket/Controls.h>

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

bool editor_gui::init(void)
{

	m_context = Rocket::Core::CreateContext("editor_gui", Rocket::Core::Vector2i(256, 256));

	Rocket::Debugger::Initialise(m_context);
	Rocket::Debugger::SetVisible(true);

	return m_objectPanel.init(m_context);
}

void editor_gui::shutdown(void)
{
	
	if (m_context)
	{
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
		m_context->ProcessTextInput(wParam);
	}
	else if (uMsg == WM_KEYDOWN)
	{
		rocket_keyboard_input_translate(m_context, wParam, lParam);
	}

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

/* object_panel */

bool object_panel::init(Rocket::Core::Context * context)
{

	m_object = nullptr;

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

void object_panel::shutdown(Rocket::Core::Context * context)
{
	context->UnloadDocument(m_panel);
}

void object_panel::set_object(renderable * object)
{
	
	m_object = object;

	if (m_object)
	{
		m_panel->Show();
	}
	else
	{
		m_panel->Hide();
	}
	
}

void object_panel::ProcessEvent(Rocket::Core::Event & event)
{

	auto element = event.GetTargetElement();
	auto document = element->GetOwnerDocument();

	if (event.GetType() == "click")
	{
		FUSE_LOG("click", event.GetTargetElement()->GetTagName().CString());
	}
	else if (event.GetType() == "change")
	{
		FUSE_LOG("change", event.GetTargetElement()->GetTagName().CString());

		auto attribute = element->GetAttribute("type");

		if (attribute->Get<Rocket::Core::String>() == "range")
		{
			Rocket::Controls::ElementFormControlInput * slider = dynamic_cast<Rocket::Controls::ElementFormControlInput*>(element);
			//element->SetInnerRML("<span class=\"range_value\">" + slider->GetValue() + "</span>");
		}
	}

}

#endif