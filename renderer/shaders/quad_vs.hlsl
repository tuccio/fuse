#include "quad_input.hlsli"

struct VSInput
{
	uint vertexID : SV_VertexID;
	uint instance : SV_InstanceID;
}; 

QuadInput quad_vs(VSInput input)
{

	QuadInput output;

	if (input.vertexID == 0)
	{
		output.positionCS = float4(-1, -1, 0, 1);
		output.texcoord   = float2(0, 1);
	}
	else if (input.vertexID == 1)
	{
		output.positionCS = float4(-1, 1, 0, 1);
		output.texcoord   = float2(0, 0);
	}
	else if (input.vertexID == 2)
	{
		output.positionCS = float4(1, -1, 0, 1);
		output.texcoord   = float2(1, 1);
	}
	else
	{
		output.positionCS = float4(1, 1, 0, 1);
		output.texcoord   = float2(1, 0);
	}
	
	output.instance   = input.instance;

	return output;

}