#ifndef __COLOR__
#define __COLOR__

float3 CIE_xyY_to_CIE_XYZ(in float3 xyY)
{

	float Yovery = xyY.z / xyY.y;
	return float3(
		xyY.x * Yovery,
		xyY.z,
		(1 - xyY.x - xyY.y) * Yovery
	);
}

float3 CIE_XYZ_to_CIE_RGB(in float3 XYZ)
{
	
	const float3x3 M = {
		 2.3706743f, -0.9000405f, -0.4706338f,
		-0.5138850f,  1.4253036f,  0.0885814f,
		 0.0052982f, -0.0146949f,  1.0093968f,
	};
	
	return mul(M, XYZ);
	
}

float3 CIE_XYZ_to_sRGB(in float3 XYZ)
{
	
	const float3x3 M = {
		 3.2404542f, -1.5371385f, -0.4985314f,
		-0.9692660f,  1.8760108f,  0.0415560f,
		 0.0556434f, -0.2040259f,  1.0572252f,
	};
	
	return mul(M, XYZ);
	
}

#endif