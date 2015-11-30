#include "quad_input.hlsli"

Texture2D    g_input   : register(t0);
SamplerState g_sampler : register(s0);

float4 composer_ps(QuadInput input) : SV_Target0
{
	float4 sample = g_input.Sample(g_sampler, input.texcoord);
	return sample;
}