#include "scene_data.hlsli"

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

USE_CB_PER_FRAME(b0)

#define USE_COLOR   (1)
#define USE_TEXTURE (2)

cbuffer cbPerObject : register(b1)
{
	float2 g_translation;
	uint   g_colorFlags;
};

PSInput rocket_vs(VSInput input)
{

	float2 position = input.position + g_translation;

	PSInput output;
	
	output.color      = input.color;
	output.texcoord   = input.texcoord;
	
	output.position = mul(float4(input.position + g_translation, 0, 1), g_screen.orthoProjection);
	
	return output;
	
}

Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 rocket_ps(PSInput input) : SV_Target0
{

	float4 color = { 1.f, 1.f, 1.f, 1.f };
	
	if (g_colorFlags & USE_TEXTURE)
	{
		float4 textureColor = g_texture.SampleLevel(g_sampler, input.texcoord, 0);
		textureColor.rgb = pow(textureColor.rgb, 1.f / 2.2f);
		color *= textureColor;
	}
	
	if (g_colorFlags & USE_COLOR)
	{
		color *= input.color;
	}
	
	return color;
	
}