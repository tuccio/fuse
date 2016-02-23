#include "quad_input.hlsli"
#include "constants.hlsli"
#include "skydome.hlsli"
#include "color.hlsli"

struct PSInput
{
	float4 positionCS : SV_Position;
	uint   cubeFace   : SV_RenderTargetArrayIndex;
	float2 texcoord   : TEXCOORD;
	float3 viewRay    : VIEWRAY;
};

/* Cubemap geometry shader */

[maxvertexcount(3)]
void skydome_gs(triangle QuadInput input[3], inout TriangleStream<PSInput> output)
{

	float3x3 orientation[] = {
	
	// x

		{
			 0,  0, -1,
			 0,  1,  0,
			 1,  0,  0
		},

	// -x

		{
			 0,  0,  1,
			 0,  1,  0,
			-1,  0,  0
		},
		
	// y

		{
			 1,  0,  0,
			 0,  0, -1,
			 0,  1,  0
		},

	// -y

		{
			 1,  0,  0,
			 0,  0,  1,
			 0, -1,  0
		},
	
	// z

		{
			 1,  0,  0,
			 0,  1,  0,
			 0,  0,  1
		},

	// -z

		{
			-1,  0,  0,
			 0,  1,  0,
			 0,  0, -1
		}
		
	};

	PSInput element;
	
	element.cubeFace = input[0].instance;
	
	[unroll]
	for (int i = 0; i < 3; i++)
	{

		element.positionCS = input[i].positionCS;
		element.texcoord   = input[i].texcoord;
		element.viewRay    = mul(input[i].viewRay, orientation[element.cubeFace]);
		
		//if (element.viewRay.y >= 0)
		{
			output.Append(element);
		}
		
	}

}


cbuffer cbSkydome : register(b0)
{

	float g_thetaSun;
	float g_cosThetaSun;
	
	float3 g_sunDirection;

	float g_zenith_Y;
	float g_zenith_x;
	float g_zenith_y;

	float g_perez_Y[5];
	float g_perez_x[5];
	float g_perez_y[5];

};

/* Cubemap pixel shader */

float4 skydome_ps(PSInput input) : SV_Target0
{
	
	if (input.viewRay.y < 0) discard;

	float3 direction = normalize(input.viewRay);
	
	float cosTheta = direction.y;
	
	float cosGamma = dot(direction, g_sunDirection);
	float gamma = acos(cosGamma);
	
	float x = skydome_perez_luminance(
		g_zenith_x,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_x[0],
		g_perez_x[1],
		g_perez_x[2],
		g_perez_x[3],
		g_perez_x[4]);
		
	float y = skydome_perez_luminance(
		g_zenith_y,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_y[0],
		g_perez_y[1],
		g_perez_y[2],
		g_perez_y[3],
		g_perez_y[4]);
		
	float Y = skydome_perez_luminance(
		g_zenith_Y,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_Y[0],
		g_perez_Y[1],
		g_perez_Y[2],
		g_perez_Y[3],
		g_perez_Y[4]);
	
	float4 color;
	
	color.rgb = CIE_XYZ_to_sRGB(CIE_xyY_to_CIE_XYZ(float3(x, y, Y)));
	color.a   = 1;
	
	return color;
	
}

struct SkydomeVSInput
{
	uint vertexID : SV_VertexID;
};

struct SkydomePSInput
{
	float4 positionCS : SV_Position;
	float2 texcoord   : TEXCOORD;
	float3 viewRay    : VIEWRAY;
};

SkydomePSInput skydome_vs(SkydomeVSInput input)
{

	SkydomePSInput output;

	if (input.vertexID == 0)
	{
		output.positionCS = float4(-1, -1, 0, 1);
		output.texcoord   = float2(0, 1);
	}
	else if (input.vertexID == 1)
	{
		output.positionCS = float4(-1, 1, 0, 1);
		output.texcoord   = float2(0, 0);
	}
	else if (input.vertexID == 2)
	{
		output.positionCS = float4(1, -1, 0, 1);
		output.texcoord   = float2(1, 1);
	}
	else
	{
		output.positionCS = float4(1, 1, 0, 1);
		output.texcoord   = float2(1, 0);
	}
	
	return output;

}

float4 skydome_nishita_ps(SkydomePSInput input) : SV_Target0
{

	float3 direction = skydome_mapping_uv_to_direction(input.texcoord);
	
	float cosTheta = direction.y;
	
	float cosGamma = dot(direction, g_sunDirection);
	float gamma = acos(cosGamma);
	
	float x = skydome_perez_luminance(
		g_zenith_x,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_x[0],
		g_perez_x[1],
		g_perez_x[2],
		g_perez_x[3],
		g_perez_x[4]);
		
	float y = skydome_perez_luminance(
		g_zenith_y,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_y[0],
		g_perez_y[1],
		g_perez_y[2],
		g_perez_y[3],
		g_perez_y[4]);
		
	float Y = skydome_perez_luminance(
		g_zenith_Y,
		cosTheta,
		gamma,
		cosGamma,
		g_thetaSun,
		g_cosThetaSun,
		g_perez_Y[0],
		g_perez_Y[1],
		g_perez_Y[2],
		g_perez_Y[3],
		g_perez_Y[4]);
	
	float4 color;
	
	color.rgb = CIE_XYZ_to_sRGB(CIE_xyY_to_CIE_XYZ(float3(x, y, Y)));
	color.a   = 1;
	
	return color;
	
}