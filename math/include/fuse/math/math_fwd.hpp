#pragma once

#include <xmmintrin.h>

namespace fuse
{

	template <typename T, int N, int M> struct matrix;

	typedef matrix<float, 2, 2> float2x2;
	typedef matrix<float, 3, 3> float3x3;
	typedef matrix<float, 4, 4> float4x4;

	typedef matrix<float, 1, 2> float2;
	typedef matrix<float, 1, 3> float3;
	typedef matrix<float, 1, 4> float4;

	typedef matrix<int, 1, 2> int2;
	typedef matrix<int, 1, 3> int3;
	typedef matrix<int, 1, 4> int4;

	typedef matrix<unsigned int, 1, 2> uint2;
	typedef matrix<unsigned int, 1, 3> uint3;
	typedef matrix<unsigned int, 1, 4> uint4;

	typedef matrix<float, 2, 1> float2col;
	typedef matrix<float, 3, 1> float3col;
	typedef matrix<float, 4, 1> float4col;

	typedef matrix<int, 2, 1> int2col;
	typedef matrix<int, 3, 1> int3col;
	typedef matrix<int, 4, 1> int4col;

	typedef matrix<unsigned int, 2, 1> uint2col;
	typedef matrix<unsigned int, 3, 1> uint3col;
	typedef matrix<unsigned int, 4, 1> uint4col;

	typedef __m128 vec128;

	struct mat128;
	struct mat128_f32;

}