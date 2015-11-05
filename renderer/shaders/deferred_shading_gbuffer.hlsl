#include "scene_data.hlsli"
#include "gbuffer.hlsli"

cbuffer cbPerFrame : register(b0)
{
	camera   g_camera;
};

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
	
	// TODO: Tangent space should be calculated only if needed (normal mapping, ...)
	
	return output;

}

gbuffer_out gbuffer_ps(PSInput input)
{

	float3 baseColor;
	
	// TODO: texture mapping etc	
	baseColor = g_material.baseColor;
	
	gbuffer      data = (gbuffer) 0;
	gbuffer_out  output;
	
	data.normal = normalize(input.normal);
	data.metallic = g_material.metallic;
	data.specular = g_material.specular;
	data.roughness = g_material.roughness;
	data.baseColor = baseColor;
	
	gbuffer_write(data, output);

	// PSOutput output = (PSOutput) 0;
	
	// //output.gbuffer2.xyz = input.position;

	
	// output.gbuffer0.xyz = .5f + .5f * normalize(input.normal);
	// output.gbuffer0.w   = g_material.metallic;
	// output.gbuffer1.w   = g_material.specular;
	// output.gbuffer2.xyz = baseColor;
	// output.gbuffer2.w   = g_material.roughness;
	
	// gbuffer will contain the lightmap uvs (maybe material id too)
	
	return output;

}