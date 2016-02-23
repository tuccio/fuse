#include "scene_data.hlsli"

USE_CB_PER_FRAME(b0)

/* Debug Lines rendering */

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

/* Debug Texture rendering */

Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct DebugTextureVSInput
{
	float2 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

struct DebugTexturePSInput
{
	float4 positionCS : SV_Position;
	float2 texcoord   : TEXCOORD0;
};

DebugTexturePSInput debug_texture_vs(DebugTextureVSInput input)
{
	DebugTexturePSInput output;
	output.positionCS = mul(float4(input.position, 0, 1), g_screen.orthoProjection);
	output.texcoord   = input.texcoord;
	return output;
}

float4 debug_texture_ps(DebugTexturePSInput input) : SV_Target
{
	float4 color = g_texture.Sample(g_sampler, input.texcoord);
	#if HDR_TEXTURE
		color.rgb /= (color.rgb + 1);
	#endif
	return color;
}