#pragma once

#include "shadow_mapping.hpp"

#include <set>

#define FUSE_RENDERER_VARIABLE(MemberType, Variable, Property, ...) \
	private: MemberType m_rvar ## Property = MemberType(__VA_ARGS__); public: \
	inline void set_ ## Property (const MemberType & value) { m_rvar ## Property = value; m_updatedVars.insert(Variable); }\
	inline MemberType get_ ## Property (void) const { return m_rvar ## Property; }

enum render_variables
{

	FUSE_RVAR_RENDER_RESOLUTION,

	FUSE_RVAR_SHADOW_MAP_RESOLUTION,

	FUSE_RVAR_SHADOW_MAPPING_ALGORITHM,

	FUSE_RVAR_VSM_MIN_VARIANCE,
	FUSE_RVAR_VSM_MIN_BLEEDING,
	FUSE_RVAR_VSM_BLUR_KERNEL_SIZE,
	FUSE_RVAR_VSM_FLOAT_PRECISION,

	FUSE_RVAR_EVSM2_MIN_VARIANCE,
	FUSE_RVAR_EVSM2_MIN_BLEEDING,
	FUSE_RVAR_EVSM2_EXPONENT,
	FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE,
	FUSE_RVAR_EVSM2_FLOAT_PRECISION,

	FUSE_RVAR_EVSM4_MIN_VARIANCE,
	FUSE_RVAR_EVSM4_MIN_BLEEDING,
	FUSE_RVAR_EVSM4_POSITIVE_EXPONENT,
	FUSE_RVAR_EVSM4_NEGATIVE_EXPONENT,
	FUSE_RVAR_EVSM4_BLUR_KERNEL_SIZE,
	FUSE_RVAR_EVSM4_FLOAT_PRECISION

};

namespace fuse
{

	class render_configuration
	{

	public:

		render_configuration(void) = default;
		render_configuration(const render_configuration &) = delete;
		render_configuration(render_configuration &&) = delete;

		std::set<render_variables> get_updates(void);

	private:

		FUSE_RENDERER_VARIABLE(XMUINT2, FUSE_RVAR_RENDER_RESOLUTION, render_resolution, 1280, 720)

		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_SHADOW_MAP_RESOLUTION, shadow_map_resolution, 1024)

		FUSE_RENDERER_VARIABLE(shadow_mapping_algorithm, FUSE_RVAR_SHADOW_MAPPING_ALGORITHM, shadow_mapping_algorithm, FUSE_SHADOW_MAPPING_EVSM2)

		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_VSM_MIN_VARIANCE,     vsm_min_variance,     .001f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_VSM_MIN_BLEEDING,     vsm_min_bleeding,     .55f)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_VSM_BLUR_KERNEL_SIZE, vsm_blur_kernel_size, 5)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_VSM_FLOAT_PRECISION,  vsm_float_precision,  16)

		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_MIN_VARIANCE,     evsm2_min_variance,     .001f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_MIN_BLEEDING,     evsm2_min_bleeding,     .55f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_EXPONENT,         evsm2_exponent,         5.f)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE, evsm2_blur_kernel_size, 5)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_EVSM2_FLOAT_PRECISION,  evsm2_float_precision,  16)

		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM4_MIN_VARIANCE,      evsm4_min_variance,     .001f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM4_MIN_BLEEDING,      evsm4_min_bleeding,     .55f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM4_POSITIVE_EXPONENT, evsm4_positive_exponent, 7.5f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM4_NEGATIVE_EXPONENT, evsm4_negative_exponent, 3.5f)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_EVSM4_BLUR_KERNEL_SIZE,  evsm4_blur_kernel_size,  5)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_EVSM4_FLOAT_PRECISION,   evsm4_float_precision,   16)

		std::set<render_variables> m_updatedVars;

	};

}