#include "shadow_mapping.hpp"

namespace fuse
{

	XMMATRIX sm_directional_light_matrix(const XMFLOAT3 & lightDirection, renderable_iterator begin, renderable_iterator end)
	{

		XMVECTOR direction = to_vector(lightDirection);

		XMMATRIX viewMatrix = XMMatrixLookAtLH(
			XMVectorZero(),
			direction,
			XMVectorSet(lightDirection.y > .99f ? 1.f : 0.f, lightDirection.y > .99f ? 0.f : 1.f, 0.f, 0.f));

		aabb currentAABB = aabb::from_min_max(
			XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX),
			XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));

		for (auto it = begin; it != end; it++)
		{
			XMMATRIX worldView = XMMatrixMultiply((*it)->get_world_matrix(), viewMatrix);
			sphere transformedSphere = transform_affine((*it)->get_bounding_sphere(), worldView);
			aabb objectAABB = bounding_aabb(transformedSphere);
			currentAABB = currentAABB + objectAABB;
		}

		auto aabbMin = to_float3(currentAABB.get_min());
		auto aabbMax = to_float3(currentAABB.get_max());

		XMMATRIX orthoMatrix = XMMatrixOrthographicOffCenterLH(aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMax.z, aabbMin.z);

		return XMMatrixMultiply(viewMatrix, orthoMatrix);

	}

}