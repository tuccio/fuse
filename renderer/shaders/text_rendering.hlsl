#include "scene_data.hlsli"

cbuffer cbPerFrame : register(b0)
{
	camera g_camera;
	screen g_screen;
};

struct VSInput
{
	float2 position : POSITION;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_Position;
	float4 color    : COLOR;
	float2 texcoord : TEXCOORD;
};

Texture2D    g_font        : register(t0);
SamplerState g_fontSampler : register(s0);

PSInput text_vs(VSInput input)
{

	PSInput output;
	
	output.color    = input.color;
	output.texcoord = input.texcoord;
	output.position = mul(float4(input.position, 0, 1), g_screen.orthoProjection);

	return output;
	
}

float4 text_ps(PSInput input) : SV_Target0
{
	float alpha = g_font.Sample(g_fontSampler, input.texcoord).a;
	return float4(input.color.rgb, alpha);
}