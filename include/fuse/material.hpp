#pragma once

#include <fuse/resource.hpp>
#include <fuse/properties_macros.hpp>

#include <string>

enum shading_model
{
	FUSE_SHADING_MODEL_UNKNOWN,
	FUSE_SHADING_MODEL_PHONG,
	FUSE_SHADING_MODEL_BLINN_PHONG,
	FUSE_SHADING_MODEL_OREN_NAYAR,
	FUSE_SHADING_MODEL_COOK_TORRANCE,
};
namespace fuse
{

	union color_rgb
	{
		struct
		{
			float r, g, b;
		};
		float v[3];
	};

	class material :
		public resource
	{

	public:

		material(void);
		material(const char * name, resource_loader * loader, resource_manager * owner);

		inline bool has_diffuse_texture(void) const { return !m_diffuseTexture.empty(); }
		inline bool has_specular_texture(void) const { return !m_specularTexture.empty(); }
		inline bool has_normal_map(void) const { return !m_normalMap.empty(); }

		void set_default(void);

	protected:

		bool   load_impl(void) override;
		void   unload_impl(void) override;
		size_t calculate_size_impl(void) override;

	private:

		color_rgb    m_diffuseAlbedo;
		color_rgb    m_specularAlbedo;

		color_rgb    m_emissive;

		float        m_metallic;
		float        m_roughness;
		float        m_subsurface;
		float        m_specularPower;

		unsigned int m_materialType;
		
		std::string  m_diffuseTexture;
		std::string  m_specularTexture;
		std::string  m_normalMap;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(diffuse_albedo,  m_diffuseAlbedo)
			(specular_albedo, m_specularAlbedo)
			(emissive,        m_emissive)
		)

		FUSE_PROPERTIES_BY_VALUE(
			(metallic,       m_metallic)
			(roughness,      m_roughness)
			(subsurface,     m_subsurface)
			(specular_power, m_specularPower)
			(material_type,  m_materialType)
		)

		FUSE_PROPERTIES_STRING(
			(diffuse_texture,  m_diffuseTexture)
			(specular_texture, m_specularTexture)
			(normal_map,       m_normalMap)
		)
		
	};

}