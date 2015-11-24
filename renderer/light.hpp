#pragma once

#include <fuse/color.hpp>
#include <fuse/math.hpp>

namespace fuse
{

	enum light_type
	{
		FUSE_LIGHT_TYPE_DIRECTIONAL,
		FUSE_LIGHT_TYPE_SPOTLIGHT,
		FUSE_LIGHT_TYPE_POINTLIGHT
	};

	enum shadow_mapping_algorithm
	{
		FUSE_SHADOW_MAPPING_NONE,
		FUSE_SHADOW_MAPPING_VSM,
		FUSE_SHADOW_MAPPING_EVSM2
	};

	struct light
	{

		light_type type;

		color_rgb color;
		float     intensity;

		XMFLOAT3  ambient;
		XMFLOAT3  direction;

		float spotAngle;

		shadow_mapping_algorithm shadowMappingAlgorithm;

	};

}