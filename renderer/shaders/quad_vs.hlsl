#include "quad_input.hlsli"

#ifndef QUAD_Z
#define QUAD_Z 0
#endif

struct VSInput
{

	uint vertexID : SV_VertexID;

#ifdef QUAD_INSTANCE_ID
	uint instance : SV_InstanceID;
#endif

}; 

QuadInput quad_vs(VSInput input)
{

	QuadInput output;

	if (input.vertexID == 0)
	{
		output.positionCS = float4(-1, -1, QUAD_Z, 1);
		output.texcoord   = float2(0, 1);
	}
	else if (input.vertexID == 1)
	{
		output.positionCS = float4(-1, 1, QUAD_Z, 1);
		output.texcoord   = float2(0, 0);
	}
	else if (input.vertexID == 2)
	{
		output.positionCS = float4(1, -1, QUAD_Z, 1);
		output.texcoord   = float2(1, 1);
	}
	else
	{
		output.positionCS = float4(1, 1, QUAD_Z, 1);
		output.texcoord   = float2(1, 0);
	}
	
#ifdef QUAD_INSTANCE_ID
	output.instance   = input.instance;
#endif

#ifdef QUAD_VIEW_RAY
	output.viewRay = float3(output.positionCS.xy, 1);
#endif

	return output;

}