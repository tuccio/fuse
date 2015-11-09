#include "scene_data.hlsli"
#include "quad_input.hlsli"
#include "gbuffer.hlsli"

cbuffer cbPerFrame : register(b0)
{
	camera g_camera;
	screen g_screen;
};

cbuffer cbPerLight : register(b1)
{
	light g_light;
};

Texture2D        g_gbuffer0 : register(t0);
Texture2D        g_gbuffer1 : register(t1);
Texture2D        g_gbuffer2 : register(t2);
Texture2D<float> g_gbuffer3 : register(t3);

SamplerState     g_pointSampler : register(s0);

float4 shading_ps(QuadInput input) : SV_Target0
{

	gbuffer_in gbuffer = { g_gbuffer0, g_gbuffer1, g_gbuffer2, g_gbuffer3 };
	
	gbuffer_data data;

	gbuffer_read(gbuffer, g_pointSampler, input.texcoord, data);
	
	float3 lightDirection = normalize(float3(-.35, 0.65, -1));
	
	//float3 luminance = dot(data.normal, lightDirection) * data.baseColor;
	float NoL = max(0, dot(data.normal, g_light.direction));
	float3 luminance = NoL * data.baseColor * g_light.luminance;
	//float3 luminance = data.baseColor;
	return float4(luminance, 1);

}