#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	struct cb_camera
	{
		float3 position;
		float fovy;

		float aspectRatio;
		float znear;
		float zfar;

		float __fill0;

		mat128 view;
		mat128 projection;
		mat128 viewProjection;
		mat128 invViewProjection;
	};

	struct cb_screen
	{
		uint2    resolution;
		uint32_t __fill0[2];
		mat128   orthoProjection;
	};

	struct cb_render_variables
	{
		uint32_t shadowMapResolution;

		float vsmMinVariance;
		float vsmMinBleeding;

		float evsm2MinVariance;
		float evsm2MinBleeding;
		float evsm2Exponent;

		float evsm4MinVariance;
		float evsm4MinBleeding;
		float evsm4PosExponent;
		float evsm4NegExponent;

		//float __fill0[1];
	};

	struct cb_material
	{
		float3 baseColor;
		float metallic;

		float roughness;
		float specular;

		float __fill[2];
	};

	struct cb_transform
	{
		mat128 world;
		mat128 worldView;
		mat128 worldViewProjection;
	};

	struct cb_light
	{
		uint32_t type;
		uint32_t castShadows;
		uint32_t __fill0[2];

		float3 luminance;
		float __fill1;

		float3 ambient;
		float __fill2;

		float3 position;
		float __fill3;

		float3 direction;
		float spotAngle;
	};

	struct cb_shadowmapping
	{
		mat128 lightMatrix;
	};

	struct cb_per_frame
	{
		cb_camera camera;
		cb_screen screen;

		cb_render_variables rvars;
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

