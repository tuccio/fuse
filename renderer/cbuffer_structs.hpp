#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	struct cb_camera
	{

		XMFLOAT3 position;
		float fovy;

		float aspectRatio;
		float znear;
		float zfar;

		float __fill0;

		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX viewProjection;
		XMMATRIX invViewProjection;

	};

	struct cb_screen
	{
		XMUINT2  resolution;
		uint32_t __fill0[2];
		XMMATRIX orthoProjection;
	};

	struct cb_material
	{

		XMFLOAT3 baseColor;
		float metallic;

		float roughness;
		float specular;

		float __fill[2];

	};

	struct cb_transform
	{
		XMMATRIX world;
		XMMATRIX worldView;
		XMMATRIX worldViewProjection;
	};

	struct cb_light
	{

		uint32_t type;
		uint32_t castShadows;
		uint32_t __fill0[2];

		XMFLOAT3 luminance;
		float __fill1;

		XMFLOAT3 ambient;
		float __fill2;

		XMFLOAT3 position;
		float __fill3;

		XMFLOAT3 direction;
		float spotAngle;

	};

	struct cb_shadowmapping
	{
		XMMATRIX lightMatrix;
	};

	struct cb_per_frame
	{
		cb_camera camera;
		cb_screen screen;
	};

	struct cb_per_object
	{
		cb_transform transform;
		cb_material  material;
	};

	struct cb_per_light
	{
		cb_light         light;
		cb_shadowmapping shadowMapping;
	};

}

