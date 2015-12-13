struct min_max
{
	float min;
	float max;
};

Texture2D<float> g_depthBuffer : register(t0);
RWStructuredBuffer<float4> g_constantBuffer : register(u0);

[numthreads(1, 1, 1)]
void min_max_reduction_cs(uint3 gid : SV_GroupID, uint3 tid : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID, uint gidx : SV_GroupIndex)
{
	g_constantBuffer[0] = float4(1, 0, 0, 1);
}

// RWStructuredBuffer<min_max> g_output : register(u0);



// [numthreads(THREADX, THREADY, 1)]
// void min_max_reduction_cs(uint3 gid : SV_GroupID, uint3 tid : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID, uint gidx : SV_GroupIndex)
// {

	// float sample = g_depthBuffer[tid.xy];
	
	

// }