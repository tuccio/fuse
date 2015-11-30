#ifndef __EVSM4__
#define __EVSM4__

#include "vsm.hlsli"
#include "esm.hlsli"

float evsm4_visibility(in float4 moments, in float depth, in float positiveExponent, in float negativeExponent, in float minVariance, in float minBleeding)
{

	float wDepthPos = esm_warp(depth, positiveExponent);
	float wDepthNeg = - esm_warp(depth, - negativeExponent);
	
	float depthScalePos = minVariance * positiveExponent * wDepthPos;
	float depthScaleNeg = minVariance * negativeExponent * wDepthNeg;
	
	float scaledMinVariancePos = depthScalePos * depthScalePos;
	float scaledMinVarianceNeg = depthScaleNeg * depthScaleNeg;
	
	float pPos = vsm_chebyshev_upperbound(moments.xy, wDepthPos, scaledMinVariancePos, minBleeding);
	float pNeg = vsm_chebyshev_upperbound(moments.zw, wDepthNeg, scaledMinVarianceNeg, minBleeding);
	
	return min(pPos, pNeg);
	
}

float4 evsm4_moments(in float depth, in float positiveExponent, in float negativeExponent)
{
	float wDepthPos = esm_warp(depth, positiveExponent);
	float wDepthNeg = - esm_warp(depth, - negativeExponent);
	return float4( vsm_moments(wDepthPos), vsm_moments(wDepthNeg) );
}

#endif
