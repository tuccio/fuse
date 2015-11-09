Texture2D           g_input  : register(t0);
RWTexture2D<float4> g_output : register(u0);

[numthreads(32, 32, 1)]
void tonemap_cs(uint3 tid : SV_DispatchThreadID)
{

	float3 color = g_input[tid.xy].rgb;
	
	float3 tonemappedColor = color / (1 + color);
	
	g_output[tid.xy] = float4(tonemappedColor, 1);

}