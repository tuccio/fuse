#include <fuse/material.hpp>

using namespace fuse;

material::material(void) { }

material::material(const char * name, resource_loader * loader, resource_manager * owner) :
	resource(name, loader, owner) { }

bool material::load_impl(void)
{
	// TODO: implement own loader
	return false;
}

void material::unload_impl(void)
{
	set_default();
}

size_t material::calculate_size_impl(void)
{
	return 1;
}

void material::set_default(void)
{

	m_baseAlbedo     = color_rgb{ .85f, .75f, .67f };
	m_diffuseAlbedo  = color_rgb{ .85f, .75f, .67f };
	m_specularAlbedo = color_rgb{ .85f, .85f, .85f };
	m_emissive       = color_rgb{};
	
	m_specular       = 1.f;
	m_metallic       = 0.f;
	m_roughness      = 0.4f;
	m_subsurface     = 0.f;

	m_materialType   = 0;

	m_diffuseTexture.clear();
	m_specularTexture.clear();
	m_normalMap.clear();
	
}