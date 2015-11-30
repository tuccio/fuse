#pragma once

#include <fuse/color.hpp>
#include <fuse/math.hpp>

enum light_type
{
	FUSE_LIGHT_TYPE_DIRECTIONAL,
	FUSE_LIGHT_TYPE_SPOTLIGHT,
	FUSE_LIGHT_TYPE_POINTLIGHT
};

namespace fuse
{

	struct light
	{

		light_type type;

		color_rgb color;
		float     intensity;

		XMFLOAT3  ambient;
		XMFLOAT3  direction;

		float spotAngle;

	};

}