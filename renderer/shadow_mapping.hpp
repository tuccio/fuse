#pragma once

#include <algorithm>

#include <fuse/render_resource.hpp>

#include "scene.hpp"

enum shadow_mapping_algorithm
{
	FUSE_SHADOW_MAPPING_NONE,
	FUSE_SHADOW_MAPPING_VSM,
	FUSE_SHADOW_MAPPING_EVSM2,
	FUSE_SHADOW_MAPPING_EVSM4
};

namespace fuse
{

	struct alignas(16) shadow_map_info
	{
		mat128 lightMatrix;

		const render_resource * shadowMap;

		D3D12_GPU_VIRTUAL_ADDRESS sdsmCBAddress;
		bool                      sdsm;
	};

	inline float esm_clamp_exponent_positive(float exponent, unsigned int precision)
	{
		return precision == 16 ? std::min(exponent, 10.f) : std::min(exponent, 42.f);
	}

	inline float esm_clamp_exponent(float exponent, unsigned int precision)
	{
		float absExp = std::abs(exponent);
		float clampedValue = precision == 16 ? std::min(absExp, 10.f) : std::min(absExp, 42.f);
		return std::copysign(clampedValue, exponent);
	}

	mat128 sm_crop_matrix_lh(const mat128 & viewMatrix, renderable_iterator begin, renderable_iterator end);

}