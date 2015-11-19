#ifdef FUSE_USE_EDITOR_GUI

#include "editor_gui.hpp"

#include <fuse/rocket_input.hpp>
#include <algorithm>

using namespace fuse;

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
	m_selectedObject = nullptr;
	return true;
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
	
	auto oldDim = m_context->GetDimensions();
	assert(m_context && "Can't call editor_gui::on_resize before initialization.");
	m_context->SetDimensions(Rocket::Core::Vector2i(width, height));

	for (auto panel : m_panels)
	{

		auto & box = panel->GetBox(0);

		auto offset = panel->GetAbsoluteOffset();
		auto size   = panel->GetBox(0).GetSize();

		auto bottomRight = offset + size;

		if (bottomRight.x > width)
		{
			offset.x = width - size.x;
		}

		if (bottomRight.y > height)
		{
			offset.y = 0.f, height - size.y;
		}

		panel->SetOffset(offset, nullptr);

	}
	
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
	if (m_selectedObject)
	{
		auto it = std::find(m_panels.begin(), m_panels.end(), m_selectedObject);
		m_panels.erase(it);
		m_context->UnloadDocument(m_selectedObject);
	}

	//m_context->LoadDocument("ui/demo/demo.rml")->Show();

	if (m_selectedObject = m_context->LoadDocument("ui/selected_object.rml"))
	{
		m_selectedObject->GetElementById("title")->SetInnerRML(m_selectedObject->GetTitle());
		m_selectedObject->Show();
		m_panels.push_back(m_selectedObject);
	}

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

#endif