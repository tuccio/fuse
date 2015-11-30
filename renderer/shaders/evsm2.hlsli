#ifndef __EVSM2__
#define __EVSM2__

#include "vsm.hlsli"
#include "esm.hlsli"

float evsm2_visibility(in float2 moments, in float depth, in float exponent, in float minVariance, in float minBleeding)
{

	float wDepth = esm_warp(depth, exponent);
	
	float depthScale = minVariance * exponent * wDepth;
	float scaledMinVariance = depthScale * depthScale;
	
	return vsm_chebyshev_upperbound(moments, wDepth, scaledMinVariance, minBleeding);
	
}

float2 evsm2_moments(in float depth, in float exponent)
{
	float wDepth = esm_warp(depth, exponent);
	//return float2(wDepth, wDepth * wDepth);
	return vsm_moments(wDepth);
}

#endif
