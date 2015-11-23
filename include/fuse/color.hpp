#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	union color_rgb
	{

		color_rgb(void) = default;
		color_rgb(const color_rgb &) = default;
		color_rgb(color_rgb &&) = default;
		color_rgb(float r, float g, float b)
		{
			this->r = r;
			this->g = g;
			this->b = b;
		}

		color_rgb & operator= (const color_rgb &) = default;

		struct
		{
			float r, g, b;
		};

		float v[3];

		inline color_rgb operator* (float s) const { return color_rgb(r * s, g * s, b * s); }
		inline color_rgb operator/ (float s) const { return color_rgb(r / s, g / s, b / s); }

		inline color_rgb operator* (const color_rgb & color) const { return color_rgb(r * color.r, g * color.g, b * color.b); }
		inline color_rgb operator+ (const color_rgb & color) const { return color_rgb(r + color.r, g + color.g, b + color.b); }
		inline color_rgb operator- (const color_rgb & color) const { return color_rgb(r - color.r, g - color.g, b - color.b); }

	};

	//inline color_rgb operator* (const color_rgb & color, float s) { return color_rgb(color.r * s, color.g * s, color.b * s); }
	//inline color_rgb operator/ (const color_rgb & color, float s) { return color_rgb(s / color.r, s / color.g, s / color.b); }

	inline XMFLOAT3 to_float3(const color_rgb & color) { return XMFLOAT3(color.r, color.g, color.b); }

}