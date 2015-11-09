#ifndef __GBUFFER__
#define __GBUFFER__

struct gbuffer_out
{
	float4 gbuffer0 : SV_Target0;
	float4 gbuffer1 : SV_Target1;
	float4 gbuffer2 : SV_Target2;
};

struct gbuffer_in
{
	Texture2D        gbuffer0;
	Texture2D        gbuffer1;
	Texture2D        gbuffer2;
	Texture2D<float> gbufferDepth;
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

}

void gbuffer_read(in gbuffer_in input, in SamplerState pointSampler, in float2 uv, out gbuffer_data data)
{

	data = (gbuffer_data) 0;

	float4 gbuffer0 = input.gbuffer0.Sample(pointSampler, uv);
	float4 gbuffer1 = input.gbuffer1.Sample(pointSampler, uv);
	float4 gbuffer2 = input.gbuffer2.Sample(pointSampler, uv);
	float  depth    = input.gbufferDepth.Sample(pointSampler, uv);

	data.normal     = (gbuffer0.xyz - .5f) * 2.f;
	data.specular   = gbuffer0.w;
	
	data.lightmapUV = gbuffer1.xy;
	data.metallic   = gbuffer1.w;
	
	data.baseColor  = gbuffer2.xyz;
	data.roughness  = gbuffer2.w;
	
	data.depth      = depth;
	
}

#endif
