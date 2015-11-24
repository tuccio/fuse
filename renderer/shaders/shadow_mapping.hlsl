#include "scene_data.hlsli"
#include "vsm.hlsli"
#include "evsm2.hlsli"

USE_CB_PER_FRAME(b0)

cbuffer cbPerObject : register(b0)
{
	shadow_mapping g_shadowMapInfo;
}

struct PSInput
{
	float4 positionCS : SV_Position;
};

PSInput shadow_map_vs(float3 position : POSITION)
{
	PSInput output;
	output.positionCS = mul(float4(position, 1), g_shadowMapInfo.lightMatrix);
	return output;
}

float2 vsm_ps(PSInput input) : SV_Target0
{
	float depth = input.positionCS.z / input.positionCS.w;
	return vsm_moments(depth);
}

float2 evsm2_ps(PSInput input) : SV_Target0
{
	float depth = input.positionCS.z / input.positionCS.w;
	return evsm2_moments(depth, R.evsm2Exponent);
}