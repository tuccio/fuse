#pragma once

#include <set>

#define FUSE_RENDERER_VARIABLE(MemberType, Variable, Property, DefaultValue) \
	private: MemberType m_p ## Property = DefaultValue; public: \
	inline void set_ ## Property (const MemberType & value) { m_p ## Property = value; m_updatedVars.insert(Variable); }\
	inline MemberType get_ ## Property (void) const { return m_p ## Property; }

enum render_variables
{
	FUSE_VAR_SHADOW_MAP_RESOLUTION,
	FUSE_VAR_VSM_MIN_VARIANCE,
	FUSE_VAR_VSM_MIN_BLEEDING
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

		FUSE_RENDERER_VARIABLE(float, FUSE_VAR_VSM_MIN_VARIANCE, vsm_min_variance, .001f)
		FUSE_RENDERER_VARIABLE(float, FUSE_VAR_VSM_MIN_BLEEDING, vsm_min_bleeding, .55f)

		std::set<render_variables> m_updatedVars;

	};

}