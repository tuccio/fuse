#ifndef __EVSM2__
#define __EVSM2__

#include "vsm.hlsli"
#include "scene_data.hlsli"

float evsm2_visibility(in float2 moments, in float4 lightSpacePosition, in float exponent, in float minVariance, in float minBleeding)
{
	float  depth = lightSpacePosition.z / lightSpacePosition.w;
	return vsm_chebyshev_upperbound(moments, depth, minVariance, minBleeding);
}

#endif
