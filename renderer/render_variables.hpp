#pragma once

#include <set>

#define FUSE_RENDERER_VARIABLE(MemberType, Variable, Property, DefaultValue) \
	private: MemberType m_rvar ## Property = DefaultValue; public: \
	inline void set_ ## Property (const MemberType & value) { m_rvar ## Property = value; m_updatedVars.insert(Variable); }\
	inline MemberType get_ ## Property (void) const { return m_rvar ## Property; }

enum render_variables
{

	FUSE_RVAR_SHADOW_MAP_RESOLUTION,

	FUSE_RVAR_VSM_MIN_VARIANCE,
	FUSE_RVAR_VSM_MIN_BLEEDING,
	FUSE_RVAR_VSM_BLUR_KERNEL_SIZE,

	FUSE_RVAR_EVSM2_MIN_VARIANCE,
	FUSE_RVAR_EVSM2_MIN_BLEEDING,
	FUSE_RVAR_EVSM2_EXPONENT,
	FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE

};

namespace fuse
{

	class renderer_configuration
	{

	public:

		renderer_configuration(void) = default;
		renderer_configuration(const renderer_configuration &) = delete;
		renderer_configuration(renderer_configuration &&) = delete;

		std::set<render_variables> get_updates(void);

	private:

		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_SHADOW_MAP_RESOLUTION, shadow_map_resolution, 1024)

		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_VSM_MIN_VARIANCE,      vsm_min_variance,      .001f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_VSM_MIN_BLEEDING,      vsm_min_bleeding,      .55f)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_VSM_BLUR_KERNEL_SIZE,  vsm_blur_kernel_size,  5)

		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_MIN_VARIANCE,     evsm2_min_variance,      .001f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_MIN_BLEEDING,     evsm2_min_bleeding,      .55f)
		FUSE_RENDERER_VARIABLE(float,    FUSE_RVAR_EVSM2_EXPONENT,         evsm2_exponent,          25.f)
		FUSE_RENDERER_VARIABLE(uint32_t, FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE, evsm2_blur_kernel_size,  5)

		std::set<render_variables> m_updatedVars;

	};

}