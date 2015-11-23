#include "scene_data.hlsli"
#include "quad_input.hlsli"
#include "gbuffer.hlsli"

#include "evsm2.hlsli"

USE_CB_PER_FRAME(b0)

cbuffer cbPerLight : register(b1)
{
	light          g_light;
	shadow_mapping g_shadowMapping;
};

Texture2D g_gbuffer0  : register(t0);
Texture2D g_gbuffer1  : register(t1);
Texture2D g_gbuffer2  : register(t2);
Texture2D g_gbuffer3  : register(t3);

Texture2D g_shadowMap : register(t4);

SamplerState g_pointSampler  : register(s0);
SamplerState g_linearSampler : register(s1);

SamplerState g_shadowMapSampler : register(s1);

float4 shading_ps(QuadInput input) : SV_Target0
{

	gbuffer_in gbuffer = { g_gbuffer0, g_gbuffer1, g_gbuffer2, g_gbuffer3 };
	
	gbuffer_data data;

	gbuffer_read(gbuffer, g_pointSampler, input.texcoord, data);
	gbuffer_position_from_depth(input.texcoord, g_camera, data);
	
	float4 lightSpacePosition = mul(float4(data.position, 1), g_shadowMapping.lightMatrix);
	//lightSpacePosition /= lightSpacePosition.w;
	
	float2 shadowMapUV = { .5f + lightSpacePosition.x * .5f, .5f - lightSpacePosition.y * .5f };
	
	//return float4(data.position, 1);
	//return float4(shadowMapUV, 0, 1);
	
	float2 moments     = g_shadowMap.Sample(g_shadowMapSampler, shadowMapUV).xy;
	
	float visibility = vsm_visibility(moments, lightSpacePosition, R.vsmMinVariance, R.vsmMinBleeding);
	
	float3 lightDirection = normalize(float3(-.35, 0.65, -1));
	
	//float3 luminance = dot(data.normal, lightDirection) * data.baseColor;
	float NoL = max(0, dot(data.normal, g_light.direction));
	float3 luminance = visibility * NoL * data.baseColor * g_light.luminance;
	//float3 luminance = data.baseColor;
	return float4(luminance, 1);

}