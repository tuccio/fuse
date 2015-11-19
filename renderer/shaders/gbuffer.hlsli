#ifndef __GBUFFER__
#define __GBUFFER__

#include "scene_data.hlsli"

struct gbuffer_out
{
	float4 gbuffer0 : SV_Target0;
	float4 gbuffer1 : SV_Target1;
	float4 gbuffer2 : SV_Target2;
	float4 gbuffer3 : SV_Target3;
};

struct gbuffer_in
{
	Texture2D gbuffer0;
	Texture2D gbuffer1;
	Texture2D gbuffer2;
	Texture2D gbuffer3;
};

struct gbuffer_data
{
	float3 position;
	float3 normal;
	float  depth;
	float3 baseColor;
	float  metallic;
	float  roughness;
	float  specular;
	float2 lightmapUV;
};

/*
 * Function to pack 32 bit floats into RGBA channels to store depth in RGBA8 RTs
 * Source: http://aras-p.info/blog/2009/07/30/encoding-floats-to-rgba-the-final/
 */

float4 gbuffer_pack_depth(in float v)
{
	float4 enc = float4(1.f, 255.f, 65025.f, 160581375.f) * v;
	enc = frac(enc);
	enc -= enc.yzww * float4(1.f / 255.f, 1.f / 255.f, 1.f / 255.f, 0.f);
	return enc;
}

float gbuffer_unpack_depth(float4 rgba)
{
	 return dot(rgba, float4(1.f, 1.f / 255.f, 1.f / 65025.f, 1.f / 160581375.f));
}

void gbuffer_write(in gbuffer_data data, out gbuffer_out output)
{

	output.gbuffer0.xyz = .5f * data.normal + .5f;
	output.gbuffer0.xyz = data.normal;
	output.gbuffer0.w   = data.specular;
	
	output.gbuffer1.xy  = data.lightmapUV;
	output.gbuffer1.z   = 0; // ?? maybe material id
	output.gbuffer1.w   = data.metallic;
	
	output.gbuffer2.xyz = data.baseColor;
	output.gbuffer2.w   = data.roughness;
	
	//output.gbuffer3     = gbuffer_pack_depth(data.depth);
	output.gbuffer3.xyz = data.position;
	output.gbuffer3.w   = 0;

}

void gbuffer_read(in gbuffer_in input, in SamplerState pointSampler, in float2 uv, out gbuffer_data data)
{

	data = (gbuffer_data) 0;

	float4 gbuffer0 = input.gbuffer0.Sample(pointSampler, uv);
	float4 gbuffer1 = input.gbuffer1.Sample(pointSampler, uv);
	float4 gbuffer2 = input.gbuffer2.Sample(pointSampler, uv);
	float4 gbuffer3 = input.gbuffer3.Sample(pointSampler, uv);

	data.normal     = (gbuffer0.xyz - .5f) * 2.f;
	data.specular   = gbuffer0.w;
	
	data.lightmapUV = gbuffer1.xy;
	data.metallic   = gbuffer1.w;
	
	data.baseColor  = gbuffer2.xyz;
	data.roughness  = gbuffer2.w;
	
	//data.depth      = gbuffer_unpack_depth(gbuffer3);
	data.position   = gbuffer3.xyz;
	
}

void gbuffer_position_from_depth(in float2 uv, in camera camera, inout gbuffer_data data)
{

	// float2 ndc = { 2 * uv.x - 1, 1 - 2 * uv.y };

	// float4 nearH = mul(float4(ndc, 0, 1), camera.invViewProjection);

	// //float3 near = nearH.xyz;
	// //near.z /= nearH.w;
	
	// float3 near = nearH.xyz / nearH.w;

	// float3 rayDir = normalize(near - camera.position);
	
	// //data.position = camera.position + rayDir * data.depth;
	// data.position = data.depth;

	// if (data.depth == 0)
	// {
		// data.position = 0;
	// }
	
	//data.position = .5f * rayDir + .5f;
	//data.position = data.depth;

	// float4 nearClipPoint = { uv, 0, 1 };
	// float4 nearClipPointH = mul(nearClipPoint, camera.invViewProjection);
	
	// nearClipPointH.z /= nearClipPointH.w;
	
	// float3 rayDir = normalize(nearClipPointH.xyz - camera.position);
	
	// data.position = camera.position + rayDir * data.depth;
	
}

void gbuffer_depth_from_position(in camera camera, inout gbuffer_data data)
{
	data.depth = length(data.position - camera.position);
}

#endif
