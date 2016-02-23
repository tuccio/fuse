#ifndef __SKYBOX__
#define __SKYBOX__

#include "constants.hlsli"

/* Clear sky Perez luminance function */

float skydome_perez_function(
	in float cosTheta,
	in float gamma,
	in float cosGamma,
	in float A,
	in float B,
	in float C,
	in float D,
	in float E)
{
	return (1 + A * exp(B / cosTheta)) * (1 + C * exp(D * gamma) + E * cosGamma * cosGamma);
}

float skydome_perez_luminance(
	in float zenithLuminance,
	in float cosTheta,
	in float gamma,
	in float cosGamma,
	in float thetaSun,
	in float cosThetaSun,
	in float A,
	in float B,
	in float C,
	in float D,
	in float E)
{
	return zenithLuminance *
		skydome_perez_function(cosTheta, gamma, cosGamma, A, B, C, D, E) /
		skydome_perez_function(1, thetaSun, cosThetaSun, A, B, C, D, E);
}

/* Clear sky CIE luminance function */

float skydome_cie_function(
	in float cosTheta,
	in float gamma,
	in float cosGamma)
{
	return (.91f - exp(-.32f / cosTheta)) * (.91f + 10 * exp(-3 * gamma) + .45f * cosGamma * cosGamma);
}

float skydome_cie_luminance(
	in float zenithLuminance,
	in float cosTheta,
	in float gamma,
	in float cosGamma,
	in float thetaSun,
	in float cosThetaSun)
{
	return zenithLuminance *
		skydome_cie_function(cosTheta, gamma, cosGamma) /
		skydome_cie_function(1, thetaSun, cosThetaSun);
}

float2 skydome_mapping_direction_to_uv(in float3 direction)
{

	float theta = acos(direction.y);
	float phi   = atan2(direction.z, direction.x);
	
	float2 uv;
	
	uv.x = phi / (2 * PI) + .5f;
	uv.y = theta / (.5f * PI);
	
	return uv;
	
}

float3 skydome_mapping_uv_to_direction(in float2 uv)
{

	float theta = uv.y * .5f * PI;
	float phi   = (uv.x - .5f) * 2 * PI;
	
	float sinTheta, cosTheta;
	float sinPhi, cosPhi;
	
	sincos(theta, sinTheta, cosTheta);
	sincos(phi, sinPhi, cosPhi);
	
	float3 direction;
	
	direction.y = cosTheta;
	direction.x = sinTheta * cosPhi;
	direction.z = sinTheta * sinPhi;
	
	return direction;

}

#endif