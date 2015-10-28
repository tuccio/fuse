cbuffer cbCamera : register(b0)
{
	float4x4 g_view;
	float4x4 g_projection;
	float4x4 g_viewProjection;
};

struct VSInput
{
	float3 position : POSITION;
};

struct PSInput
{
	float4 position : SV_Position;
};

PSInput main_vs(VSInput input)
{
	PSInput output;
	output.position = mul(float4(input.position, 1), g_viewProjection);
	//output.position = float4(input.position, 1);
	return output;
}

float4 main_ps(void) : SV_Target0
{
	return float4(1, 1, 1, 1);
}