#ifndef __QUAD_INPUT__
#define __QUAD_INPUT__

struct QuadInput
{
	float4 positionCS : SV_Position;
	float2 texcoord   : TEXCOORD;
	
#ifdef QUAD_INSTANCE_ID
	uint   instance   : SV_InstanceID;
#endif
	
#ifdef QUAD_VIEW_RAY
	float3 viewRay : VIEWRAY;
#endif

};

#endif