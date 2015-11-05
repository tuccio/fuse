#include "scene.hpp"

#include <assimp/scene.h>

#include <fuse/resource_factory.hpp>

#include <stack>

using namespace fuse;

static XMMATRIX to_xmmatrix(const aiMatrix4x4 & matrix)
{

	XMMATRIX result;

	for (int i = 0; i < 4; i++)
	{
		result.r[i] = XMLoadFloat4(reinterpret_cast<const XMFLOAT4 *>(matrix[i]));
	}

	return result;

};

template <typename Visitor>
void visit_assimp_scene_dfs(const aiScene * scene, Visitor visitor)
{

	std::stack<aiNode *> stack;
	std::stack<XMMATRIX> parentTransform;

	stack.push(scene->mRootNode);
	parentTransform.push(DirectX::XMMatrixIdentity());

	do
	{

		aiNode * node = stack.top();

		XMMATRIX world      = parentTransform.top();
		XMMATRIX local      = XMMatrixTranspose(to_xmmatrix(node->mTransformation)); // Transpose because assimp uses column vector algebra
		XMMATRIX worldLocal = local * world;

		if (!visitor(scene, node, world, local, worldLocal))
		{
			return;
		}

		stack.pop();
		parentTransform.pop();

		for (int i = 0; i < node->mNumChildren; i++)
		{
			stack.push(node->mChildren[i]);
			parentTransform.push(worldLocal);
		}

	} while (!stack.empty());

}


bool scene::import_static_objects(assimp_loader * loader)
{

	if (!loader->is_loaded())
	{
		return false;
	}

	const aiScene * scene = loader->get_scene();

	std::stack<aiNode *> stack;
	std::stack<XMMATRIX> parentTransform;

	stack.push(scene->mRootNode);
	parentTransform.push(DirectX::XMMatrixIdentity());

	visit_assimp_scene_dfs(scene, 
		[&] (const aiScene * scene, const aiNode * node, const XMMATRIX & world, const XMMATRIX & local, const XMMATRIX & worldLocal)
	{

		for (int i = 0; i < node->mNumMeshes; i++)
		{

			unsigned int meshIndex = node->mMeshes[i];
			unsigned int materialIndex = scene->mMeshes[meshIndex]->mMaterialIndex;

			mesh_ptr nodeMesh = loader->create_mesh(meshIndex);

			if (!nodeMesh || !nodeMesh->load())
			{
				return false;
			}

			if (!nodeMesh->has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
			{
				nodeMesh->calculate_tangent_space();
			}

			// TODO: remove phony texcoords to handle the objects with no diffuse texture (add shaders permutations)

			if (nodeMesh->add_storage_semantic(FUSE_MESH_STORAGE_TEXCOORDS0))
			{
				FUSE_LOG_OPT("assimp_loader", "Added phony texcoords to mesh.");
			}

			gpu_mesh_ptr nodeGPUMesh  = resource_factory::get_singleton_pointer()->create<gpu_mesh>("gpu_mesh", nodeMesh->get_name());
			material_ptr nodeMaterial = loader->create_material(materialIndex);

			if (!nodeGPUMesh || !nodeGPUMesh->load() ||
				!nodeMaterial || !nodeMaterial->load())
			{
				return false;
			}

			renderable object;

			object.set_world_matrix(worldLocal);
			object.set_material(nodeMaterial);
			object.set_mesh(nodeGPUMesh);

			m_staticObjects.push_back(object);

		}

		return true;

	}
	);

	return true;

}

static bool find_camera_node(const aiScene * scene, aiCamera * camera, aiNode ** outNode, XMMATRIX * outTransform)
{

	*outNode = nullptr;

	visit_assimp_scene_dfs(scene,
		[&] (const aiScene * scene, aiNode * node, const XMMATRIX & world, const XMMATRIX & local, const XMMATRIX & worldLocal)
	{

		if (node->mName == camera->mName)
		{
			*outNode = node;
			*outTransform = world;
			return false;
		}

		return true;

	}
	);

	return *outNode != nullptr;

}

bool scene::import_cameras(assimp_loader * loader)
{

	if (!loader->is_loaded())
	{
		return false;
	}

	const aiScene * scene = loader->get_scene();

	for (int i = 0; i < scene->mNumCameras; i++)
	{
			
		camera camera;

		aiNode * node;
		XMMATRIX transform;

		XMFLOAT3 position      = reinterpret_cast<const XMFLOAT3&>(scene->mCameras[i]->mPosition);
		XMFLOAT3 up            = reinterpret_cast<const XMFLOAT3&>(scene->mCameras[i]->mUp);
		XMFLOAT3 viewDirection = reinterpret_cast<const XMFLOAT3&>(scene->mCameras[i]->mLookAt);
		
		position.z = -position.z;

		XMFLOAT3 lookAt = to_float3(to_vector(position) - to_vector(viewDirection));

		if (find_camera_node(scene, scene->mCameras[i], &node, &transform))
		{
			position = to_float3(XMVector3Transform(to_vector(position), transform));
			up       = to_float3((XMVector3TransformNormal(to_vector(up), transform)));
			lookAt   = to_float3(XMVector3Transform(to_vector(lookAt), transform));
		}

		camera.look_at(position, up, lookAt);

		camera.set_znear(scene->mCameras[i]->mClipPlaneNear);
		camera.set_zfar(scene->mCameras[i]->mClipPlaneFar);
		//camera.set_aspect_ratio(scene->mCameras[i]->mAspect);
		//camera.set_fovx(scene->mCameras[i]->mHorizontalFOV);

		m_cameras.push_back(camera);

	}

	if (!m_cameras.empty() && !m_activeCamera)
	{
		m_activeCamera = &m_cameras.front();
	}

	return true;

}

std::pair<scene::renderable_iterator, scene::renderable_iterator> scene::get_static_objects_iterators(void)
{
	return std::make_pair(m_staticObjects.begin(), m_staticObjects.end());
}

std::pair<scene::camera_iterator, scene::camera_iterator> scene::get_cameras_iterators(void)
{
	return std::make_pair(m_cameras.begin(), m_cameras.end());
}