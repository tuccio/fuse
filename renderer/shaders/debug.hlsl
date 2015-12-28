#include "scene_data.hlsli"

USE_CB_PER_FRAME(b0)

cbuffer cbPerObject : register(b1)
{
	float4 g_color;
};

struct VSInput
{
	float3 position : POSITION;
};

struct PSInput
{
	float4 positionCS : SV_Position;
};

PSInput debug_vs(VSInput input)
{
	PSInput output;
	output.positionCS = mul(input.position, g_camera.viewProjection);
	return output;
}

float4 debug_ps(PSInput input) : SV_Target
{
	return float4(0, 1, 0, 1);
	//return g_color;
}