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

[numthreads(32, 32, 1)]
void box_blur_cs(uint3 tid : SV_DispatchThreadID)
{

	TYPE color = (TYPE) 0;
	
	float2 pixelUV = (.5f + tid.xy) / float2(WIDTH, HEIGHT);

	for (int i = KERNEL_START; i <= KERNEL_END; i++)
	{
		float2 uv = pixelUV + OFFSET(i * INV_SIZE);
		TYPE sampledColor = g_input.SampleLevel(g_sampler, uv, 0);
		color += sampledColor;
	}
	
	g_output[tid.xy] = color / KERNEL_SIZE;

}