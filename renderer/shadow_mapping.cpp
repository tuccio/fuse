#include "shadow_mapping.hpp"

namespace fuse
{

	XMMATRIX sm_crop_matrix_lh(const XMMATRIX & viewMatrix, renderable_iterator begin, renderable_iterator end)
	{

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

		return XMMatrixOrthographicOffCenterLH(aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMax.z, aabbMin.z);

	}
}