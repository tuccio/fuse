#include "scene_data.hlsli"

USE_CB_PER_FRAME(b0)

struct VSInput
{
	float3 position : POSITION;
	float4 color    : COLOR;
};

struct PSInput
{
	float4 positionCS : SV_Position;
	float4 color      : COLOR;
};

PSInput debug_vs(VSInput input)
{
	PSInput output;
	output.positionCS = mul(float4(input.position, 1), g_camera.viewProjection);
	output.color      = input.color;
	return output;
}

float4 debug_ps(PSInput input) : SV_Target
{
	return input.color;
}