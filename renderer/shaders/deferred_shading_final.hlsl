#include "scene_data.hlsli"
#include "quad_input.hlsli"
#include "gbuffer.hlsli"

#include "evsm2.hlsli"
#include "evsm4.hlsli"

#include "skydome.hlsli"

USE_CB_PER_FRAME(b0)

cbuffer cbPerLight : register(b1)
{
	light          g_light;
	shadow_mapping g_shadowMapping;
};

cbuffer cbShadowMapAlgorithm : register(b2)
{
	uint g_shadowMapAlgorithm;
};

cbuffer cbSDSM : register(b3)
{
	float4 g_sdsm;
};

Texture2D g_gbuffer0  : register(t0);
Texture2D g_gbuffer1  : register(t1);
Texture2D g_gbuffer2  : register(t2);
Texture2D g_gbuffer3  : register(t3);

Texture2D g_shadowMap : register(t4);

#if (LIGHT_TYPE == LIGHT_TYPE_SKYDOME)
Texture2D g_skydome : register(t5);
#endif

SamplerState g_pointSampler       : register(s0);
SamplerState g_linearSampler      : register(s1);
SamplerState g_anisotropicSampler : register(s2);

SamplerState g_shadowMapSampler : register(s2);

/* Shading */

float4 shading_ps(QuadInput input) : SV_Target0
{

	gbuffer_in gbuffer = { g_gbuffer0, g_gbuffer1, g_gbuffer2, g_gbuffer3 };
	
	gbuffer_data data;

	gbuffer_read(gbuffer, g_pointSampler, input.texcoord, data);
	gbuffer_position_from_depth(input.texcoord, g_camera, data);
	
	float visibility;
	
	#if (SHADOW_MAPPING_ALGORITHM == SHADOW_MAPPING_NONE)
	{
		visibility = 1.f;
	}
	#else
	{
	
		float4 lightSpacePosition = mul(float4(data.position, 1), g_shadowMapping.lightMatrix);
		float  lightSpaceDepth = lightSpacePosition.z / lightSpacePosition.w;
		
		float2 shadowMapUV = { .5f + lightSpacePosition.x * .5f, .5f - lightSpacePosition.y * .5f };
		
		//float lod = g_shadowMap.CalculateLevelOfDetail(g_shadowMapSampler, shadowMapUV);
		
		#if (SHADOW_MAPPING_ALGORITHM == SHADOW_MAPPING_VSM)
		{
			float2 moments = g_shadowMap.SampleLevel(g_anisotropicSampler, shadowMapUV, 0).xy;
			visibility = vsm_visibility(moments, lightSpaceDepth, R.vsmMinVariance, R.vsmMinBleeding);
		}
		#elif (SHADOW_MAPPING_ALGORITHM == SHADOW_MAPPING_EVSM2)
		{
			float2 moments = g_shadowMap.SampleLevel(g_anisotropicSampler, shadowMapUV, 0).xy;
			visibility = evsm2_visibility(moments, lightSpaceDepth, R.evsm2Exponent, R.evsm2MinVariance, R.evsm2MinBleeding);
		}
		#elif (SHADOW_MAPPING_ALGORITHM == SHADOW_MAPPING_EVSM4)
		{
			float4 moments = g_shadowMap.SampleLevel(g_anisotropicSampler, shadowMapUV, 0);
			visibility = evsm4_visibility(moments, lightSpaceDepth, R.evsm4PosExponent, R.evsm4NegExponent, R.evsm4MinVariance, R.evsm4MinBleeding);
		}
		#endif
		
	}
	#endif
	
	float NoL = max(0, dot(data.normal, g_light.direction));
	
	float3 diffuse;
	
	#if (LIGHT_TYPE == LIGHT_TYPE_DIRECTIONAL) || (LIGHT_TYPE == LIGHT_TYPE_SKYDOME)
	{
		diffuse = NoL * data.baseColor * g_light.luminance;
	}
	#endif
	
	#if (LIGHT_TYPE == LIGHT_TYPE_SKYDOME)
		//diffuse = g_sdsm;
	#endif
	
	float3 luminance = visibility * diffuse + g_light.ambient * data.baseColor;
	
	return float4(luminance, 1);

}

#ifdef QUAD_VIEW_RAY

float4 skydome_ps(QuadInput input) : SV_Target0
{
	
	float4 farH = mul(float4(input.viewRay.xyz, 1), g_camera.invViewProjection);
	float3 eyeRayWS = normalize(farH.xyz / farH.w - g_camera.position);
	
	float2 texcoord = skydome_mapping_direction_to_uv(eyeRayWS);
	
	return g_skydome.SampleLevel(g_linearSampler, texcoord, 0);
	
}

#endif
