#include "shadow_mapping.hpp"

namespace fuse
{

	mat128 sm_crop_matrix_lh(const mat128 & viewMatrix, renderable_iterator begin, renderable_iterator end)
	{
		aabb currentAABB = aabb::from_min_max(
			vec128_set(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX),
			vec128_set(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX));

		for (auto it = begin; it != end; it++)
		{
			mat128 worldView = (*it)->get_world_matrix() * viewMatrix;
			sphere transformedSphere = transform_affine((*it)->get_bounding_sphere(), worldView);
			aabb objectAABB = bounding_aabb(transformedSphere);
			currentAABB = currentAABB + objectAABB;
		}

		auto aabbMin = to_float3(currentAABB.get_min());
		auto aabbMax = to_float3(currentAABB.get_max());

		return to_mat128(ortho_lh(aabbMin.x, aabbMax.x, aabbMin.y, aabbMax.y, aabbMax.z, aabbMin.z));
	}
}