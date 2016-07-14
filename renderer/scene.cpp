#include "scene.hpp"

#include <fuse/resource_factory.hpp>

#include <iterator>
#include <stack>

using namespace fuse;

#define OCTREE_MAX_DEPTH 9

FUSE_DEFINE_ALIGNED_ALLOCATOR_NEW(scene, 16)

static const float3 & to_float3(const aiVector3D & color)
{
	return reinterpret_cast<const float3 &>(color);

}
static const float3 & to_float3(const aiColor3D & color)
{
	return reinterpret_cast<const float3 &>(color);
}

static const color_rgb & to_color_rgb(const aiColor3D & color)
{
	return reinterpret_cast<const color_rgb &>(color);

}
static mat128 to_mat128(const aiMatrix4x4 & matrix)
{
	/*XMMATRIX result;

	for (int i = 0; i < 4; i++)
	{
		result.r[i] = XMLoadFloat4(reinterpret_cast<const XMFLOAT4 *>(matrix[i]));
	}

	return result;*/
	return mat128_load(reinterpret_cast<const float4x4&>(matrix));
};

template <typename Visitor>
void visit_assimp_scene_dfs(const aiScene * scene, Visitor visitor)
{
	std::stack<aiNode*> stack;
	std::stack<mat128, std::deque<mat128, aligned_allocator<mat128>>> parentTransform;

	stack.push(scene->mRootNode);
	parentTransform.push(mat128_identity());

	do
	{
		aiNode * node = stack.top();

		mat128 world = parentTransform.top();
		mat128 local = to_mat128(node->mTransformation);

		mat128 worldLocal = local * world;

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

void scene::create_scene_graph_assimp(assimp_loader * loader, const aiScene * scene, aiNode * node, scene_graph_node * parent)
{
	if (node)
	{
		scene_graph_node * newNode = nullptr;

		if (node->mNumMeshes)
		{
			newNode = parent->add_child<scene_graph_geometry>();
		}
		else
		{
			newNode = parent->add_child<scene_graph_group>();
		}

		if (newNode)
		{
			string_t name = to_string_t(node->mName.C_Str());

			float4x4 m = reinterpret_cast<const float4x4&>(node->mTransformation);

			quaternion r;
			float3 s, t;
			decompose_affine(m, &s, &r, &t);

			r = normalize(r);

			newNode->set_name(name.c_str());

			newNode->set_local_rotation(r);
			newNode->set_local_translation(t);
			newNode->set_local_scale(s);

			newNode->set_name(to_string_t(node->mName.C_Str()).c_str());

			for (int i = 0; i < node->mNumChildren; ++i)
			{
				create_scene_graph_assimp(loader, scene, node->mChildren[i], newNode);
			}

			if (newNode->get_type() == FUSE_SCENE_GRAPH_GEOMETRY)
			{
				scene_graph_geometry * gNode = static_cast<scene_graph_geometry*>(newNode);

				unsigned int meshIndex     = node->mMeshes[0];
				unsigned int materialIndex = scene->mMeshes[meshIndex]->mMaterialIndex;

				mesh_ptr nodeMesh = loader->create_mesh(meshIndex);

				if (nodeMesh && nodeMesh->load())
				{

					if (!nodeMesh->has_storage_semantic(FUSE_MESH_STORAGE_TANGENTS))
					{
						nodeMesh->calculate_tangent_space();
					}

					// TODO: remove phony texcoords to handle the objects with no diffuse texture (add shaders permutations)

					if (nodeMesh->add_storage_semantic(FUSE_MESH_STORAGE_TEXCOORDS0))
					{
						FUSE_LOG_OPT(FUSE_LITERAL("assimp_loader"), FUSE_LITERAL("Added phony texcoords to mesh."));
					}

					gpu_mesh_ptr nodeGPUMesh = resource_factory::get_singleton_pointer()->create<gpu_mesh>(FUSE_RESOURCE_TYPE_GPU_MESH, nodeMesh->get_name());
					material_ptr nodeMaterial = loader->create_material(materialIndex);

					if (nodeGPUMesh && nodeGPUMesh->load() &&
					    nodeMaterial && nodeMaterial->load())
					{
						sphere s = bounding_sphere(
							reinterpret_cast<const float3 *>(nodeMesh->get_vertices()),
							reinterpret_cast<const float3 *>(nodeMesh->get_vertices()) + nodeMesh->get_num_vertices());

						gNode->set_local_bounding_sphere(s);

						gNode->set_gpu_mesh(nodeGPUMesh);
						gNode->set_material(nodeMaterial);

						m_geometry.push_back(gNode);
					}
				}

				on_geometry_add(static_cast<scene_graph_geometry*>(newNode));
			}
		}
	}
}

bool scene::import_static_objects(assimp_loader * loader)
{
	if (!loader->is_loaded())
	{
		return false;
	}

	const aiScene * scene = loader->get_scene();
	create_scene_graph_assimp(loader, scene, scene->mRootNode, m_sceneGraph.get_root());
	return true;
}

static bool find_node_by_name(const aiScene * scene, const aiString & name, aiNode ** outNode, mat128 * outTransform)
{
	*outNode = nullptr;

	visit_assimp_scene_dfs(scene,
		[&] (const aiScene * scene, aiNode * node, const mat128 & world, const mat128 & local, const mat128 & worldLocal)
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
		mat128 transform;

		float3 position      = to_float3(scene->mCameras[i]->mPosition);
		float3 up            = to_float3(scene->mCameras[i]->mUp);
		float3 viewDirection = to_float3(scene->mCameras[i]->mLookAt);
		
		position.z = -position.z;

		float3 lookAt = position - viewDirection;

		if (find_node_by_name(scene, scene->mCameras[i]->mName, &node, &transform))
		{
			position = to_float3(mat128_transform3(to_vec128(position), transform));
			up       = to_float3((mat128_transform_normal(to_vec128(up), transform)));
			lookAt   = to_float3(mat128_transform3(to_vec128(lookAt), transform));
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
		mat128 transform;

		if (scene->mLights[i]->mType == aiLightSource_DIRECTIONAL &&
		    find_node_by_name(scene, scene->mLights[i]->mName, &node, &transform))
		{
			vec128 direction = vec128_normalize3(mat128_transform3(to_vec128(to_float3(scene->mLights[i]->mDirection)), transform));

			light * light = new ::light;

			light->type      = FUSE_LIGHT_TYPE_DIRECTIONAL;
			light->ambient   = to_color_rgb(scene->mLights[i]->mColorAmbient);
			light->color     = to_color_rgb(scene->mLights[i]->mColorDiffuse);
			light->direction = vec128_checksign<1, 1, 1>(vec128_eq(direction, vec128_zero())) ? float3(0, 1, 0) : to_float3(direction);

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

std::pair<geometry_iterator, geometry_iterator> scene::get_geometry(void)
{
	return std::make_pair(m_geometry.begin(), m_geometry.end());
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
	for (auto r : m_geometry)
	{
		delete r;
	}

	m_geometry.clear();
	m_sgOctree.clear();
}

void scene::recalculate_octree(void)
{
	m_sgOctree = geometry_octree(m_sceneBounds.get_center(), vec128_get_x(m_sceneBounds.get_half_extents()), OCTREE_MAX_DEPTH);

	m_sceneGraph.visit(
		[&](scene_graph_node * n)
	{
		if (n->get_type() == FUSE_SCENE_GRAPH_GEOMETRY)
		{
			m_sgOctree.insert(static_cast<scene_graph_geometry*>(n));
		}
	});
}

geometry_vector scene::frustum_culling(const frustum & f)
{
	geometry_vector geometry;
	m_sgOctree.query(f, [&](scene_graph_geometry * r) { geometry.push_back(r); });
	return geometry;
}

void scene::draw_octree(visual_debugger * debugger)
{
	m_sgOctree.traverse(
		[=](const aabb & aabb, geometry_octree::objects_list_iterator begin, geometry_octree::objects_list_iterator end)
	{
		debugger->add(aabb, color_rgba(1, 0, 0, 1));
	});
}

aabb scene::get_fitting_bounds(void) const
{
	if (m_geometry.empty())
	{
		return aabb::from_min_max(vec128_zero(), vec128_zero());
	}

	aabb bounds = aabb::from_min_max(
		vec128_set(FLT_MAX, FLT_MAX, FLT_MAX, 1.f),
		vec128_set(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.f));

	for (scene_graph_geometry * g : m_geometry)
	{
		bounds = bounds + bounding_aabb(g->get_global_bounding_sphere());
	}

	return bounds;
}

void scene::fit_octree(float scale)
{
	m_sceneBounds = get_fitting_bounds();
	m_sceneBounds.set_half_extents(m_sceneBounds.get_half_extents() * scale);
	recalculate_octree();
}

void scene::update(void)
{
	m_sceneGraph.update();
}

void scene::on_geometry_add(scene_graph_geometry * g)
{
	g->add_listener(this);
}

void scene::on_geometry_remove(scene_graph_geometry * g)
{
	g->remove_listener(this);
}

void scene::on_scene_graph_node_move(scene_graph_node * node, const mat128 & oldTransform, const mat128 & newTransform)
{
	scene_graph_geometry * g = static_cast<scene_graph_geometry*>(node);
	const sphere & s = g->get_local_bounding_sphere();
	m_sgOctree.remove(g, transform_affine(s, oldTransform));
	m_sgOctree.insert(g);
}

bool FUSE_VECTOR_CALL scene::ray_pick(ray r, scene_graph_geometry * & node, float & t)
{
	return m_sgOctree.ray_pick(r, node, t);
}