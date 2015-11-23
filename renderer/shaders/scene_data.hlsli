#ifndef __SCENE_DATA__
#define __SCENE_DATA__

struct camera
{

	float3   position;
	
	float    fovy;
	float    aspectRatio;
	
	float    znear;
	float    zfar;

	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 invViewProjection;

};

struct screen
{
	uint2    resolution;
	float4x4 orthoProjection;
};

struct render_variables
{
	float vsmMinVariance;
	float vsmMinBleeding;
};

struct transform
{
	float4x4 world;
	float4x4 worldView;
	float4x4 worldViewProjection;
};

struct material
{
	float3 baseColor;
	float  metallic;
	float  roughness;
	float  specular;
};

struct light
{

	uint   type;
	bool   castShadows;
	
	float3 luminance;
	float3 ambient;
	
	float3 position;
	float3 direction;
	
	float  spotAngle;
	
};

struct shadow_mapping
{
	float4x4 lightMatrix;
};

#define USE_CB_PER_FRAME(Register)\
cbuffer cbPerFrame : register(Register)\
{\
	camera g_camera;\
	screen g_screen;\
	render_variables R;\
};

#endif
