#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	struct color_rgb
	{

		color_rgb(void) = default;
		color_rgb(const color_rgb &) = default;
		color_rgb(color_rgb &&) = default;
		color_rgb(float r, float g, float b) :
			r(r), g(g), b(b) {}

		color_rgb & operator= (const color_rgb &) = default;

		union
		{
			struct
			{
				float r, g, b;
			};

			float v[3];
		};

		inline color_rgb operator* (float s) const { return color_rgb(r * s, g * s, b * s); }
		inline color_rgb operator/ (float s) const { return color_rgb(r / s, g / s, b / s); }

		inline color_rgb operator* (const color_rgb & color) const { return color_rgb(r * color.r, g * color.g, b * color.b); }
		inline color_rgb operator+ (const color_rgb & color) const { return color_rgb(r + color.r, g + color.g, b + color.b); }
		inline color_rgb operator- (const color_rgb & color) const { return color_rgb(r - color.r, g - color.g, b - color.b); }

		inline operator float* () { return v; }
		inline operator const float* () const { return v; }

		/* Constants */

		static const color_rgb black;
		static const color_rgb white;
		static const color_rgb red;
		static const color_rgb green;
		static const color_rgb blue;
		static const color_rgb yellow;
		static const color_rgb purple;
		static const color_rgb cyan;

	};

	struct color_rgba
	{

		color_rgba(void) = default;
		color_rgba(const color_rgba &) = default;
		color_rgba(color_rgba &&) = default;
		color_rgba(const color_rgb & c, float a) :
			r(c.r), g(c.g), b(c.b), a(a) {}

		color_rgba(float r, float g, float b, float a) :
			r(r), g(g), b(b), a(a) {}

		color_rgba & operator= (const color_rgba &) = default;

		union
		{

			struct
			{
				float r, g, b, a;
			};

			float v[4];

		};

		inline color_rgba operator* (float s) const { return color_rgba(r * s, g * s, b * s, a * s); }
		inline color_rgba operator/ (float s) const { return color_rgba(r / s, g / s, b / s, a / s); }

		inline color_rgba operator* (const color_rgba & color) const { return color_rgba(r * color.r, g * color.g, b * color.b, a * color.a); }
		inline color_rgba operator+ (const color_rgba & color) const { return color_rgba(r + color.r, g + color.g, b + color.b, a + color.a); }
		inline color_rgba operator- (const color_rgba & color) const { return color_rgba(r - color.r, g - color.g, b - color.b, a - color.a); }

		inline operator float* () { return v; }
		inline operator const float* () const { return v; }

		/* Constants */

		static const color_rgba zero;
		static const color_rgba black;
		static const color_rgba white;
		static const color_rgba red;
		static const color_rgba green;
		static const color_rgba blue;
		static const color_rgba yellow;
		static const color_rgba purple;
		static const color_rgba cyan;

	};

	//inline color_rgb operator* (const color_rgb & color, float s) { return color_rgb(color.r * s, color.g * s, color.b * s); }
	//inline color_rgb operator/ (const color_rgb & color, float s) { return color_rgb(s / color.r, s / color.g, s / color.b); }

	inline XMFLOAT3 to_float3(const color_rgb & color) { return XMFLOAT3(color.r, color.g, color.b); }
	inline XMFLOAT4 to_float4(const color_rgba & color) { return XMFLOAT4(color.r, color.g, color.b, color.a); }

}