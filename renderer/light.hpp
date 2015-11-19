#pragma once

#include <fuse/math.hpp>

enum light_type
{
	FUSE_LIGHT_TYPE_DIRECTIONAL,
	FUSE_LIGHT_TYPE_SPOTLIGHT,
	FUSE_LIGHT_TYPE_POINTLIGHT
};

struct light
{

	light_type        type;

	DirectX::XMFLOAT3 luminance;

	DirectX::XMFLOAT3 ambient;
	DirectX::XMFLOAT3 direction;

	float             spotAngle;

};