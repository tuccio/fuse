#pragma once

#ifdef FUSE_ASSIMP

#include <fuse/material.hpp>
#include <fuse/mesh.hpp>

#include <fuse/resource.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace fuse
{

	class assimp_loader :
		public resource_loader
	{

	public:

		assimp_loader(const char_t * filename, unsigned int flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
		~assimp_loader(void);

		bool load(resource * r) override;
		void unload(resource * r) override;

		std::shared_ptr<mesh> create_mesh(unsigned int meshIndex);
		std::shared_ptr<material> create_material(unsigned int materialIndex);

		const aiScene * get_scene(void) const;

		inline bool is_loaded(void) const { return m_scene != nullptr;  }
		inline const char * get_error_string(void) const { return m_importer.GetErrorString(); }

	private:

		string_t           m_filename;
		const aiScene    * m_scene;
		Assimp::Importer   m_importer;

		bool load_mesh(mesh * m);
		void unload_mesh(mesh * m);

		bool load_material(material * m);
		void unload_material(material * m);

	};

}

#endif