#pragma once

#include <fuse/math.hpp>

enum light_type
{
	LIGHT_TYPE_DIRECTIONAL,
	LIGHT_TYPE_SPOTLIGHT,
	LIGHT_TYPE_POINTLIGHT
};

struct light
{

	light_type        type;

	DirectX::XMFLOAT3 luminance;

	DirectX::XMFLOAT3 ambient;
	DirectX::XMFLOAT3 direction;

	float             spotAngle;

};