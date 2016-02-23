#pragma once

#ifdef FUSE_USE_EDITOR_GUI

#include <fuse/directx_helper.hpp>
#include <fuse/input.hpp>

#include <Rocket/Core.h>

#include "renderable.hpp"
#include "scene.hpp"
#include "render_variables.hpp"

#include <list>

namespace fuse
{

	namespace gui
	{

		class gui_panel
		{

		public:

			gui_panel(void);
			~gui_panel(void);

			virtual void shutdown(void);

			void show(int flags = Rocket::Core::ElementDocument::FOCUS);
			void hide(void);
			bool is_visible(void) const;

		protected:

			Rocket::Core::ElementDocument * m_panel;

		public:

			FUSE_PROPERTIES_BY_VALUE_READ_ONLY(
				(panel, m_panel)
			)

		};

		class object_panel :
			public Rocket::Core::EventListener,
			public gui_panel
		{

		public:

			bool init(Rocket::Core::Context * context);

			void set_object(renderable * object);

			void ProcessEvent(Rocket::Core::Event & event) override;

		private:

			renderable * m_configuration;

		};

		class render_options :
			public Rocket::Core::EventListener,
			public gui_panel
		{

		public:

			bool init(Rocket::Core::Context * context, renderer_configuration * r);

			void ProcessEvent(Rocket::Core::Event & event) override;

		private:

			renderer_configuration * m_configuration;
			bool m_filling;

			void fill_form(void);

		};

		class color_panel :
			public Rocket::Core::EventListener,
			public gui_panel
		{

		public:

			bool init(Rocket::Core::Context * context);

			void ProcessEvent(Rocket::Core::Event & event) override;

		private:

			Rocket::Core::Colourb m_color;
			bool                  m_filling;

			void fill(void);

		public:

			FUSE_PROPERTIES_BY_CONST_REFERENCE(
				(color, m_color)
			)

		};

		class skybox_panel :
			public Rocket::Core::EventListener,
			public gui_panel
		{

		public:

			bool init(Rocket::Core::Context * context, skydome * skydome);

			void ProcessEvent(Rocket::Core::Event & event) override;

		private:

			skydome * m_skydome;
			bool     m_filling;

			void fill_form(void);

		};

		class light_panel :
			public Rocket::Core::EventListener,
			public gui_panel

		{

		public:

			bool init(Rocket::Core::Context * context, scene * r);
			void shutdown(void);

			void set_light(light * light);
			void ProcessEvent(Rocket::Core::Event & event) override;

			void next_light(void);

		private:

			scene * m_scene;
			light * m_light;
			bool    m_filling;

			color_panel m_colorPanel;

			void fill_form(void);

		};

	}

	class editor_gui :
		public mouse_listener,
		public keyboard_listener
	{

	public:

		editor_gui(void);
		editor_gui(const editor_gui &) = delete;
		editor_gui(editor_gui &&) = default;

		~editor_gui(void);
		
		bool init(scene * scene, renderer_configuration * r);
		void shutdown(void);

		void on_resize(UINT width, UINT height);

		LRESULT on_keyboard(WPARAM wParam, LPARAM lParam);
		LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		bool on_mouse_event(const mouse & mouse, const mouse_event_info & event) override;
		bool on_keyboard_event(const keyboard & mouse, const keyboard_event_info & event) override;

		void update(void);
		void render(void);

		void select_object(renderable * object);

		void set_render_options_visibility(bool visibility);
		bool get_render_options_visibility(void) const;

		void set_object_panel_visibility(bool visibility);
		bool get_object_panel_visibility(void) const;

		void set_skybox_panel_visibility(bool visibility);
		bool get_skybox_panel_visibility(void) const;

		void set_light_panel_visibility(bool visibility);
		bool get_light_panel_visibility(void) const;

		void set_debugger_visibility(bool visibility);
		bool get_debugger_visibility(void) const;

	private:

		Rocket::Core::Context * m_context;
		gui::object_panel       m_objectPanel;
		gui::skybox_panel       m_skyboxPanel;
		gui::light_panel        m_lightPanel;
		gui::render_options     m_renderOptions;

	};

}

#endif