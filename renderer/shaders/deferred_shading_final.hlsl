#include "scene_data.hlsli"
#include "quad_input.hlsli"

cbuffer cbPerFrame : register(b0)
{
	camera   g_camera;
};

Texture2D        gbuffer0 : register(t0);
Texture2D        gbuffer1 : register(t1);
Texture2D        gbuffer2 : register(t2);
Texture2D<float> gbuffer3 : register(t3);

float4 shading_ps(QuadInput input) : SV_Target0
{

	return float4(0, 0, 0, 0);

}