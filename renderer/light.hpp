#pragma once

#include <fuse/color.hpp>
#include <fuse/math.hpp>

enum light_type
{
	FUSE_LIGHT_TYPE_SKYDOME,
	FUSE_LIGHT_TYPE_DIRECTIONAL,
	FUSE_LIGHT_TYPE_SPOTLIGHT,
	FUSE_LIGHT_TYPE_POINTLIGHT
};

namespace fuse
{

	class skydome;

	struct light
	{

		light_type type;

		color_rgb color;
		float     intensity;

		color_rgb ambient;
		XMFLOAT3  direction;

		bool castShadow;

		union
		{
			float spotAngle;
			skydome * skydome;
		};


	};

}