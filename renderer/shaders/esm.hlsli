#ifndef __ESM__
#define __ESM__

float2 esm_warp(in float depth, in float positiveExponent, in float negativeExponent)
{

	float d = 2.f * depth - 1.f;
	
	float p = exp(positiveExponent * depth);
	float n = exp(- negativeExponent * depth);
	
	return float2(p, n);

}

float esm_warp(in float depth, in float exponent)
{
	float d = 2.f * depth - 1.f;
	return exp(exponent * depth);
}

#endif