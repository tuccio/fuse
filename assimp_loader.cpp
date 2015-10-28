#ifdef FUSE_ASSIMP

#include <fuse/assimp_loader.hpp>
#include <fuse/logger.hpp>

#include <fuse/resource_factory.hpp>
#include <fuse/resource_manager.hpp>
#include <fuse/mesh_manager.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <algorithm>
#include <cstring>
#include <sstream>

using namespace fuse;

assimp_loader::assimp_loader(const char * filename, unsigned int flags) :
	m_filename(filename)
{

	m_scene = m_importer.ReadFile(filename, flags | aiProcess_Triangulate);

	if (!m_scene)
	{
		FUSE_LOG_OPT_DEBUG(std::stringstream() << "Failed to load scene \"" << filename << "\": " << m_importer.GetErrorString());
	}

}

assimp_loader::~assimp_loader(void)
{
	m_importer.FreeScene();
}

bool assimp_loader::load(resource * r)
{

	const char * resourceType = r->get_owner()->get_type();

	if (!strcmp(resourceType, FUSE_RESOURCE_TYPE_MESH))
	{
		return load_mesh(static_cast<mesh *>(r));
	}
	else if (!strcmp(resourceType, FUSE_RESOURCE_TYPE_MATERIAL))
	{
		return load_material(static_cast<material *>(r));
	}

	return false;

}

void assimp_loader::unload(resource * r)
{

	const char * resourceType = r->get_owner()->get_type();

	if (!strcmp(resourceType, FUSE_RESOURCE_TYPE_MESH))
	{
		unload_mesh(static_cast<mesh *>(r));
	}
	else if (!strcmp(resourceType, FUSE_RESOURCE_TYPE_MATERIAL))
	{
		unload_material(static_cast<material *>(r));
	}

}

const aiScene * assimp_loader::get_scene(void) const
{
	return m_scene;
}

std::shared_ptr<mesh> assimp_loader::create_mesh(unsigned int meshIndex)
{
	return resource_factory::get_singleton_pointer()->create<mesh>(FUSE_RESOURCE_TYPE_MESH, (std::to_string(meshIndex) + "_assimp_mesh_" + m_filename).c_str(), default_parameters(), this);
}

std::shared_ptr<material> assimp_loader::create_material(unsigned int materialIndex)
{
	return resource_factory::get_singleton_pointer()->create<material>(FUSE_RESOURCE_TYPE_MATERIAL, (std::to_string(materialIndex) + "_assimp_material_" + m_filename).c_str(), default_parameters(), this);
}

bool assimp_loader::load_mesh(mesh * m)
{

	const char * name = m->get_name();

	unsigned int id;
	std::istringstream ss(name);
	ss >> id;

	// Look for the resource in the loaded scene

	if (ss.fail() || id >= m_scene->mNumMeshes)
	{

		id = -1;

		for (int i = 0; i < m_scene->mNumMeshes; i++)
		{
			if (!strcmp(name, m_scene->mMeshes[i]->mName.C_Str()))
			{
				id = i;
				break;
			}
		}

	}

	if (id >= 0)
	{

		// Load the mesh

		aiMesh * pMesh = m_scene->mMeshes[id];

		unsigned int flags = 0;

		if (pMesh->HasNormals()) flags |= FUSE_MESH_STORAGE_NORMALS;
		if (pMesh->HasTangentsAndBitangents()) flags |= FUSE_MESH_STORAGE_TANGENTSPACE;
		
		for (int i = 0; i < FUSE_MESH_MAX_TEXCOORDS; i++)
		{
			if (pMesh->HasTextureCoords(i)) flags |= (FUSE_MESH_STORAGE_TEXCOORDS0 << i);
		}

		m->create(pMesh->mNumVertices, pMesh->mNumFaces, flags);

		struct __triangle { uint32_t v[3]; };

		std::transform(pMesh->mFaces,
		               pMesh->mFaces + pMesh->mNumFaces,
		               (__triangle *) m->get_indices(),
		               [] (const aiFace & face) { return __triangle { face.mIndices[0], face.mIndices[1], face.mIndices[2] }; });

		memcpy(m->get_vertices(), pMesh->mVertices, pMesh->mNumVertices * sizeof(float) * 3);

		if (pMesh->HasNormals()) memcpy(m->get_normals(), pMesh->mNormals, pMesh->mNumVertices * sizeof(float) * 3);

		if (pMesh->HasTangentsAndBitangents())
		{
			memcpy(m->get_tangents(), pMesh->mTangents, pMesh->mNumVertices * sizeof(float) * 3);
			memcpy(m->get_bitangents(), pMesh->mBitangents, pMesh->mNumVertices * sizeof(float) * 3);
		}

		return true;

	}
	else
	{
		return false;
	}
	
}

void assimp_loader::unload_mesh(mesh * m)
{
	m->clear();
}

bool assimp_loader::load_material(material * m)
{
	
	const char * name = m->get_name();

	unsigned int id;
	std::istringstream ss(name);
	ss >> id;

	// Look for the resource in the loaded scene

	if (ss.fail() || id >= m_scene->mNumMeshes)
	{
		return false;
	}

	m->set_default();

	aiMaterial * pMat = m_scene->mMaterials[id];

	if (pMat->GetTextureCount(aiTextureType_DIFFUSE))
	{
		aiString path;
		pMat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
		m->set_diffuse_texture(path.C_Str());
	}

	if (pMat->GetTextureCount(aiTextureType_SPECULAR))
	{
		aiString path;
		pMat->GetTexture(aiTextureType_SPECULAR, 0, &path);
		m->set_specular_texture(path.C_Str());
	}

	if (pMat->GetTextureCount(aiTextureType_NORMALS))
	{
		aiString path;
		pMat->GetTexture(aiTextureType_NORMALS, 0, &path);
		m->set_normal_map(path.C_Str());
	}

	aiColor3D diffuseAlbedo, specularAlbedo, emissive;

	if (pMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseAlbedo))
	{
		m->set_diffuse_albedo(reinterpret_cast<color_rgb&>(diffuseAlbedo));
	}

	if (pMat->Get(AI_MATKEY_COLOR_SPECULAR, specularAlbedo))
	{
		m->set_specular_albedo(reinterpret_cast<color_rgb&>(specularAlbedo));
	}

	if (pMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive))
	{
		m->set_emissive(reinterpret_cast<color_rgb&>(emissive));
	}

	float metallic, roughness, specularPower, subsurface;

	if (pMat->Get("$mat.metallic", 0, 0, metallic) == aiReturn_SUCCESS)
	{
		m->set_roughness(metallic);
	}

	if (pMat->Get("$mat.roughness", 0, 0, roughness) == aiReturn_SUCCESS)
	{
		m->set_roughness(roughness);
	}

	if (pMat->Get(AI_MATKEY_SHININESS, specularPower) == aiReturn_SUCCESS)
	{
		m->set_specular_power(specularPower);
	}

	if (pMat->Get("$mat.subsurface", 0, 0, subsurface) == aiReturn_SUCCESS)
	{
		m->set_subsurface(subsurface);
	}

	aiShadingMode shadingModel;
	unsigned int  fuseShadingModel = FUSE_SHADING_MODEL_UNKNOWN;

	if (pMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel))
	{
		switch (shadingModel)
		{

		case aiShadingMode_CookTorrance:
			fuseShadingModel = FUSE_SHADING_MODEL_COOK_TORRANCE; break;

		case aiShadingMode_OrenNayar:
			fuseShadingModel = FUSE_SHADING_MODEL_OREN_NAYAR; break;

		case aiShadingMode_Phong:
			fuseShadingModel = FUSE_SHADING_MODEL_PHONG; break;

		case aiShadingMode_Blinn:
			fuseShadingModel = FUSE_SHADING_MODEL_BLINN_PHONG; break;

		default:
			break;
		}
	}

	m->set_material_type(fuseShadingModel);

	return true;

}

void assimp_loader::unload_material(material * m)
{

}

#endif