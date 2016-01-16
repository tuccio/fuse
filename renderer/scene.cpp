#include "scene.hpp"

#include <assimp/scene.h>

#include <fuse/resource_factory.hpp>

#include <stack>

using namespace fuse;

#define OCTREE_MAX_DEPTH 9

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(renderable, 16)

static const XMFLOAT3 & to_xmfloat3(const aiVector3D & color)
{
	return reinterpret_cast<const XMFLOAT3 &>(color);

}
static const XMFLOAT3 & to_xmfloat3(const aiColor3D & color)
{
	return reinterpret_cast<const XMFLOAT3 &>(color);
}

static const color_rgb & to_color_rgb(const aiColor3D & color)
{
	return reinterpret_cast<const color_rgb &>(color);

}
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
		//local = XMMatrixIdentity();
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

	visit_assimp_scene_dfs(scene, 
		[&] (const aiScene * scene, const aiNode * node, const XMMATRIX & world, const XMMATRIX & local, const XMMATRIX & worldLocal)
	{

		for (int i = 0; i < node->mNumMeshes; i++)
		{

			unsigned int meshIndex     = node->mMeshes[i];
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
				FUSE_LOG_OPT(FUSE_LITERAL("assimp_loader"), FUSE_LITERAL("Added phony texcoords to mesh."));
			}

			gpu_mesh_ptr nodeGPUMesh  = resource_factory::get_singleton_pointer()->create<gpu_mesh>(FUSE_RESOURCE_TYPE_GPU_MESH, nodeMesh->get_name());
			material_ptr nodeMaterial = loader->create_material(materialIndex);

			if (!nodeGPUMesh || !nodeGPUMesh->load() ||
				!nodeMaterial || !nodeMaterial->load())
			{
				return false;
			}

			sphere boundingSphere = bounding_sphere(
				reinterpret_cast<const XMFLOAT3 *>(nodeMesh->get_vertices()),
				reinterpret_cast<const XMFLOAT3 *>(nodeMesh->get_vertices()) + nodeMesh->get_num_vertices());

			renderable * object = new renderable;

			object->set_world_matrix(worldLocal);
			object->set_material(nodeMaterial);
			object->set_mesh(nodeGPUMesh);
			object->set_bounding_sphere(boundingSphere);

			m_renderableObjects.push_back(object);

		}

		return true;

	}
	);

	return true;

}

static bool find_node_by_name(const aiScene * scene, const aiString & name, aiNode ** outNode, XMMATRIX * outTransform)
{

	*outNode = nullptr;

	visit_assimp_scene_dfs(scene,
		[&] (const aiScene * scene, aiNode * node, const XMMATRIX & world, const XMMATRIX & local, const XMMATRIX & worldLocal)
	{

		if (node->mName == name)
		{
			*outNode      = node;
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
			
		camera * camera = new fuse::camera;

		aiNode * node;
		XMMATRIX transform;

		XMFLOAT3 position      = to_xmfloat3(scene->mCameras[i]->mPosition);
		XMFLOAT3 up            = to_xmfloat3(scene->mCameras[i]->mUp);
		XMFLOAT3 viewDirection = to_xmfloat3(scene->mCameras[i]->mLookAt);
		
		position.z = -position.z;

		XMFLOAT3 lookAt = to_float3(to_vector(position) - to_vector(viewDirection));

		if (find_node_by_name(scene, scene->mCameras[i]->mName, &node, &transform))
		{
			position = to_float3(XMVector3Transform(to_vector(position), transform));
			up       = to_float3((XMVector3TransformNormal(to_vector(up), transform)));
			lookAt   = to_float3(XMVector3Transform(to_vector(lookAt), transform));
		}

		camera->look_at(position, up, lookAt);

		camera->set_znear(scene->mCameras[i]->mClipPlaneNear);
		camera->set_zfar(scene->mCameras[i]->mClipPlaneFar);
		//camera.set_aspect_ratio(scene->mCameras[i]->mAspect);
		//camera.set_fovx(scene->mCameras[i]->mHorizontalFOV);

		m_cameras.push_back(camera);

	}

	if (!m_cameras.empty() && !m_activeCamera)
	{
		m_activeCamera = m_cameras.front();
	}

	return true;

}

bool scene::import_lights(assimp_loader * loader)
{

	if (!loader->is_loaded())
	{
		return false;
	}

	const aiScene * scene = loader->get_scene();

	for (int i = 0; i < scene->mNumLights; i++)
	{

		aiNode * node;
		XMMATRIX transform;

		if (scene->mLights[i]->mType == aiLightSource_DIRECTIONAL &&
		    find_node_by_name(scene, scene->mLights[i]->mName, &node, &transform))
		{

			XMVECTOR direction = XMVector3Normalize(XMVector3Transform(to_vector(to_xmfloat3(scene->mLights[i]->mDirection)), transform));

			light * light = new ::light;

			light->type      = FUSE_LIGHT_TYPE_DIRECTIONAL;
			light->ambient   = to_color_rgb(scene->mLights[i]->mColorAmbient);
			light->color     = to_color_rgb(scene->mLights[i]->mColorDiffuse);
			light->direction = XMVector3Equal(direction, XMVectorZero()) ? XMFLOAT3(0, 1, 0) : to_float3(direction);

			light->intensity = 1.f;

			float maxColor = std::max(std::max(light->color.r, light->color.g), light->color.b);

			if (maxColor > 1)
			{

				light->color.r /= maxColor;
				light->color.g /= maxColor;
				light->color.b /= maxColor;

				light->intensity = maxColor;

			}

			m_lights.push_back(light);

		}

	}
	
	return true;

}

std::pair<renderable_iterator, renderable_iterator> scene::get_renderable_objects(void)
{
	return std::make_pair(m_renderableObjects.begin(), m_renderableObjects.end());
}

std::pair<camera_iterator, camera_iterator> scene::get_cameras(void)
{
	return std::make_pair(m_cameras.begin(), m_cameras.end());
}

std::pair<light_iterator, light_iterator> scene::get_lights(void)
{
	return std::make_pair(m_lights.begin(), m_lights.end());
}

void scene::clear(void)
{

	for (auto r : m_renderableObjects)
	{
		delete r;
	}

	m_renderableObjects.clear();

}

void scene::recalculate_octree(void)
{

	m_octree = renderable_octree(m_sceneBounds.get_center(), XMVectorGetX(m_sceneBounds.get_half_extents()), OCTREE_MAX_DEPTH);

	for (renderable * r : m_renderableObjects)
	{
		m_octree.insert(r);
	}

}

renderable_vector scene::frustum_culling(const frustum & f)
{
	renderable_vector renderables;
	m_octree.query(f, [&](renderable * r) { renderables.push_back(r); });
	return renderables;
}