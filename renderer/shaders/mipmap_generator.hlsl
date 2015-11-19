#include "quad_input.hlsli"

Texture2D    g_mip0    : register(t0);
SamplerState g_sampler : register(s0);

float4 mipmap_ps(QuadInput input) : SV_Target0
{
	return g_mip0.SampleLevel(g_sampler, input.texcoord, 0);
}