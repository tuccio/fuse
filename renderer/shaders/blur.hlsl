Texture2D<TYPE>   g_input  : register(t0);
RWTexture2D<TYPE> g_output : register(u0);

SamplerState g_sampler : register(s0);

#define KERNEL_END   (KERNEL_SIZE >> 1)
#define KERNEL_START (- KERNEL_END)

#ifdef HORIZONTAL
	#define SIZE (WIDTH)
	#define INV_SIZE (1.f / WIDTH)
	#define OFFSET(Offset) (float2(Offset, 0))
#else
	#define SIZE (HEIGHT)
	#define INV_SIZE (1.f / HEIGHT)
	#define OFFSET(Offset) (float2(0, Offset))
#endif

#define GAUSS_WEIGHT(Offset) (gaussKernel[Offset + KERNEL_END])

[numthreads(32, 32, 1)]
void box_blur_cs(uint3 tid : SV_DispatchThreadID)
{

	TYPE color = (TYPE) 0;
	
	float2 pixelUV = (.5f + tid.xy) / float2(WIDTH, HEIGHT);

	[unroll]
	for (int i = KERNEL_START; i <= KERNEL_END; i++)
	{
		float2 uv = pixelUV + OFFSET(i * INV_SIZE);
		TYPE sampledColor = g_input.SampleLevel(g_sampler, uv, 0);
		color += sampledColor;
	}
	
	g_output[tid.xy] = color / KERNEL_SIZE;

}

#ifdef GAUSS_KERNEL

[numthreads(32, 32, 1)]
void gaussian_blur_cs(uint3 tid : SV_DispatchThreadID)
{

	static const float gaussKernel[KERNEL_SIZE] = { GAUSS_KERNEL };
	
	TYPE color = (TYPE) 0;
	
	float2 pixelUV = (.5f + tid.xy) / float2(WIDTH, HEIGHT);

	[unroll]
	for (int i = KERNEL_START; i <= KERNEL_END; i++)
	{
		float2 uv = pixelUV + OFFSET(i * INV_SIZE);
		TYPE sampledColor = g_input.SampleLevel(g_sampler, uv, 0);
		color += sampledColor * GAUSS_WEIGHT(i);
	}
	
	g_output[tid.xy] = color;

}

#endif

#ifdef GAUSS_KERNEL

[numthreads(32, 32, 1)]
void bilateral_blur_cs(uint3 tid : SV_DispatchThreadID)
{

	static const float gaussKernel[KERNEL_SIZE] = { GAUSS_KERNEL };
	
	float  normalization = 1.f;
	float2 pixelUV = (.5f + tid.xy) / float2(WIDTH, HEIGHT);
	
	TYPE centerColor = g_input.SampleLevel(g_sampler, pixelUV, 0);

	TYPE output = centerColor;

	// Normalize colors depending on the vector length
	float distanceNormalizationFactor = length((TYPE) 1);

	[unroll]
	for (int i = KERNEL_START; i <= KERNEL_END; i++)
	{
	
		float2 uv = pixelUV + OFFSET(i * INV_SIZE);

		TYPE sampleColor = g_input.SampleLevel(g_sampler, uv, 0);

		float closeness = length(sampleColor - centerColor) / distanceNormalizationFactor;
		float gaussian  = GAUSS_WEIGHT(i);

		float sampleWeight = closeness * gaussian;

		output += sampleWeight * sampleColor;

		normalization += sampleWeight;

	}

	g_output[tid.xy] = output / normalization;

}

#endif