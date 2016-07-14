#pragma once

#ifdef FUSE_WXWIDGETS

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <wx/clrpicker.h>

#include <functional>
#include <type_traits>

#define FUSE_WX_SLIDER_MAX 100000
#define FUSE_WX_SLIDER_MIN 0

#include <fuse/color.hpp>

namespace fuse
{
	
	namespace detail
	{

		template <typename T, typename Enable = void>
		struct wx_slider_normalizer;

		template <typename T>
		struct wx_slider_normalizer<T, std::enable_if_t<std::is_floating_point<T>::value>>
		{

			wx_slider_normalizer(T min, T max) :
				m_min(min), m_max(max) {}

		protected:

			inline T to_type(int value) const
			{
				return m_min + value * (m_max - m_min) / FUSE_WX_SLIDER_MAX;
			}

			inline int to_int(T value) const
			{
				return FUSE_WX_SLIDER_MAX * ((value - m_min) / (m_max - m_min));
			}

			inline int get_wx_min(void) const
			{
				return FUSE_WX_SLIDER_MIN;
			}

			inline int get_wx_max(void) const
			{
				return FUSE_WX_SLIDER_MAX;
			}

		private:

			T m_min;
			T m_max;

		};

		template <typename T>
		struct wx_slider_normalizer<T, std::enable_if_t<std::is_integral<T>::value>>
		{

			wx_slider_normalizer(T min, T max) :
				m_min(min), m_max(max) {}

		protected:

			inline T to_type(int value) const
			{
				return static_cast<T>(value);
			}

			inline int to_int(T value) const
			{
				return static_cast<int>(value);
			}

			inline int get_wx_min(void) const
			{
				return m_min;
			}

			inline int get_wx_max(void) const
			{
				return m_max;
			}

		private:

			T m_min;
			T m_max;

		};

	}

	template <typename T>
	class wx_slider :
		detail::wx_slider_normalizer<T>,
		public wxSlider
	{

	public:

		template <typename Functor>
		wx_slider(Functor cb, wxWindow * parent, int id, T value, T min, T max) :
			wx_slider_normalizer(min, max),
			wxSlider(parent, id, to_int(value), get_wx_min(), get_wx_max())
		{
			Bind(wxEVT_SLIDER, [=](wxCommandEvent & event) { cb(get_value()); });
		}

		inline T get_value(void) const
		{
			return to_type(GetValue());
		}

		inline void set_value(T value)
		{
			SetValue(to_int(value));
		}

	private:

		T m_min;
		T m_max;

	};

	template <typename T, typename Enable = void>
	class wx_spin;

	template <typename T>
	class wx_spin<T/*, std::enable_if_t<std::is_floating_point<T>::value>*/> :
		public wxSpinCtrlDouble
	{

	public:

		template <typename Functor>
		wx_spin(Functor cb, wxWindow * parent, int id, T value, T min, T max, T step = T(0.01)) :
			wxSpinCtrlDouble(parent, id)
		{
			if (std::is_integral<T>::value) SetDigits(0);

			SetRange(static_cast<double>(min), static_cast<double>(max));
			SetValue(static_cast<double>(value));
			SetIncrement(static_cast<double>(step));

			Bind(wxEVT_SPINCTRLDOUBLE, [=](wxSpinDoubleEvent & event) { cb(static_cast<T>(event.GetValue())); });
		}

		template <typename Getter, typename Setter>
		wx_spin(Getter get, Setter set, wxWindow * parent, int id, T min, T max, T step = T(0.01)) :
			wxSpinCtrlDouble(parent, id),
			m_getter(get)
		{
			if (std::is_integral<T>::value) SetDigits(0);

			SetRange(static_cast<double>(min), static_cast<double>(max));
			SetIncrement(static_cast<double>(step));

			update_value();

			Bind(wxEVT_SPINCTRLDOUBLE, [=](wxSpinDoubleEvent & event) { set(static_cast<T>(event.GetValue())); });
		}

		void update_value(void)
		{
			SetValue(static_cast<double>(m_getter()));
		}

	private:

		std::function<T(void)> m_getter;
	};

	/*template <typename T>
	class wx_spin<T, std::enable_if_t<std::is_integral<T>::value>> :
		public wxSpinCtrl
	{

	public:

		template <typename Functor>
		wx_spin(Functor cb, wxWindow * parent, int id, T value, T min, T max, T step = 1) :
			wxSpinCtrl(parent, id)
		{

			SetRange(static_cast<int>(min), static_cast<int>(max));
			SetValue(static_cast<int>(value));

			if (step != 1)
			{
				Bind(wxEVT_SPIN_UP, [=](wxSpinEvent & event) { FUSE_LOG(L"SUP", L"SUP"); SetValue(GetValue() + step); });
				Bind(wxEVT_SPIN_DOWN, [=](wxSpinEvent & event) { SetValue(GetValue() - step); });
			}

			Bind(wxEVT_SPINCTRL, [=](wxSpinEvent & event) { cb(static_cast<T>(event.GetValue())); });

		}

	};*/

	class wx_button :
		public wxButton
	{

	public:

		template <typename Functor>
		wx_button(Functor cb, wxWindow * parent, int id, const wxString & label) :
			wxButton(parent, id, label)
		{
			Bind(wxEVT_BUTTON, [=](wxCommandEvent & event) { cb(); });
		}

	};

	class wx_choice :
		public wxChoice
	{

	public:

		template <typename Functor, typename ... Args>
		wx_choice(Functor cb, wxWindow * parent, int id, Args && ... args)
		{
			wxString choices[] = { args ... };
			Create(parent, id, wxDefaultPosition, wxDefaultSize, _countof(choices), choices);
			Bind(wxEVT_CHOICE, [=](wxCommandEvent & event) { cb(event.GetInt()); });
		}

	};

	class wx_checkbox :
		public wxCheckBox
	{

	public:

		template <typename Functor, typename ... Args>
		wx_checkbox(Functor cb, wxWindow * parent, int id, const wxString & label, bool value) :
			wxCheckBox(parent, id, label)
		{
			Bind(wxEVT_CHECKBOX, [=](wxCommandEvent & event) { cb(GetValue()); });
			SetValue(value);
		}

	};

	class wx_color_picker :
		public wxColourPickerCtrl
	{

	public:

		template <typename Functor>
		wx_color_picker(Functor cb, wxWindow * parent, int id, const color_rgba & color)
		{
			wxColour wx(
				static_cast<unsigned char>(color.r * 255.f),
				static_cast<unsigned char>(color.g * 255.f),
				static_cast<unsigned char>(color.b * 255.f),
				static_cast<unsigned char>(color.a * 255.f));

			Create(parent, id, wx);

			Bind(wxEVT_COLOURPICKER_CHANGED, [=](wxColourPickerEvent & event)
			{
				wxColour value = event.GetColour();
				cb(color_rgba(
					value.Red() / 255.f,
					value.Green() / 255.f,
					value.Blue() / 255.f,
					value.Alpha() / 255.f));
			});
		}

		template <typename Functor>
		wx_color_picker(Functor cb, wxWindow * parent, int id, const color_rgb & color)
		{
			wxColour wx(
				static_cast<unsigned char>(color.r * 255.f),
				static_cast<unsigned char>(color.g * 255.f),
				static_cast<unsigned char>(color.b * 255.f),
				255);

			Create(parent, id, wx);

			Bind(wxEVT_COLOURPICKER_CHANGED, [=](wxColourPickerEvent & event)
			{
				wxColour value = event.GetColour();
				cb(color_rgb(
					value.Red() / 255.f,
					value.Green() / 255.f,
					value.Blue() / 255.f));
			});
		}

	};

}

#endif