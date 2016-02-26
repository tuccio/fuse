#include "render_variables.hpp"

using namespace fuse;

std::set<render_variables> render_configuration::get_updates(void)
{
	std::set<render_variables> updates;
	updates.swap(m_updatedVars);
	return updates;
}