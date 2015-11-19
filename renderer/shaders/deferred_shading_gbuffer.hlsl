#include "scene_data.hlsli"
#include "gbuffer.hlsli"

USE_CB_PER_FRAME(b0)

cbuffer cbPerObject : register(b1)
{
	transform g_transform;
	material  g_material;
};

struct VSInput
{
	float3 position  : POSITION;
	float3 normal    : NORMAL;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float2 texcoord  : TEXCOORD0;
};

struct PSInput
{
	float4 positionCS : SV_Position;
	float3 position   : POSITION;
	float3 normal     : NORMAL;
	float3 tangent    : TANGENT;
	float3 bitangent  : BITANGENT;
	float2 texcoord   : TEXCOORD0;
};

struct PSOutput
{
	float4 gbuffer0 : SV_Target0;
	float4 gbuffer1 : SV_Target1;
	float4 gbuffer2 : SV_Target2;
};

PSInput gbuffer_vs(VSInput input)
{

	PSInput output = (PSInput) 0;
	
	float4 positionH = float4(input.position, 1);
	
	output.positionCS  = mul(positionH, g_transform.worldViewProjection);
	output.position    = mul(positionH, g_transform.world).xyz;
	
	output.normal    = mul(input.normal,    (float3x3) g_transform.world);
	output.tangent   = mul(input.tangent,   (float3x3) g_transform.world);
	output.bitangent = mul(input.bitangent, (float3x3) g_transform.world);
	output.texcoord  = input.texcoord;
	
	return output;

}

gbuffer_out gbuffer_ps(PSInput input)
{

	float3 baseColor;
	
	// TODO: texture mapping etc	
	baseColor = g_material.baseColor;
	
	gbuffer_data data = (gbuffer_data) 0;
	gbuffer_out  output;
	
	data.normal    = normalize(input.normal);
	data.metallic  = g_material.metallic;
	data.specular  = g_material.specular;
	data.roughness = g_material.roughness;
	data.baseColor = baseColor;
	data.position  = input.position;
	
	gbuffer_depth_from_position(g_camera, data);	
	gbuffer_write(data, output);
	
	return output;

}