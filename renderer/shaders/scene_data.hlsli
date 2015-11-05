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
	float3 luminance;
	float3 ambient;
	float3 position;
	float3 direction;
	float  spotAngle;
};

#endif
