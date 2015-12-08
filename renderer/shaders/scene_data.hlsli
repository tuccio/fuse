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

	uint  shadowMapResolution;
	
	float vsmMinVariance;
	float vsmMinBleeding;
	
	float evsm2MinVariance;
	float evsm2MinBleeding;
	float evsm2Exponent;
	
	float evsm4MinVariance;
	float evsm4MinBleeding;
	float evsm4PosExponent;
	float evsm4NegExponent;
	
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

#define SHADOW_MAPPING_NONE  0
#define SHADOW_MAPPING_VSM   1
#define SHADOW_MAPPING_EVSM2 2
#define SHADOW_MAPPING_EVSM4 3

#define LIGHT_TYPE_SKYBOX      0
#define LIGHT_TYPE_DIRECTIONAL 1
#define LIGHT_TYPE_SPOTLIGHT   2
#define LIGHT_TYPE_POINTLIGHT  3 

#endif
