#include "deferred_renderer.hpp"

#include <algorithm>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/descriptor_heap.hpp>

#include "cbuffer_structs.hpp"

using namespace fuse;

deferred_renderer::deferred_renderer(void) { }

bool deferred_renderer::init(ID3D12Device * device, const deferred_renderer_configuration & cfg)
{

	m_configuration = cfg;

	return set_shadow_mapping_algorithm(m_configuration.shadowMappingAlgorithm) &&
		create_psos(device);

}

void deferred_renderer::shutdown(void)
{

	//m_gbufferPSO.reset();
	//m_gbufferRS.reset();

	m_gbufferPST.clear();
	m_shadingPST.clear();

	//m_directionalPSO.reset();
	m_gbufferRS.reset();
	m_shadingRS.reset();

}

bool deferred_renderer::set_shadow_mapping_algorithm(shadow_mapping_algorithm algorithm)
{
	
	m_configuration.shadowMappingAlgorithm = algorithm;

	switch (algorithm)
	{

	case FUSE_SHADOW_MAPPING_NONE:
		m_shadowMapAlgorithmDefine = "SHADOW_MAPPING_NONE";
		break;

	case FUSE_SHADOW_MAPPING_VSM:
		m_shadowMapAlgorithmDefine = "SHADOW_MAPPING_VSM";
		break;

	case FUSE_SHADOW_MAPPING_EVSM2:
		m_shadowMapAlgorithmDefine = "SHADOW_MAPPING_EVSM2";
		break;

	case FUSE_SHADOW_MAPPING_EVSM4:
		m_shadowMapAlgorithmDefine = "SHADOW_MAPPING_EVSM4";
		break;

	}

	return true;
}

void deferred_renderer::render_gbuffer(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	render_resource * const * gbuffer,
	const render_resource & depthBuffer,
	const camera * camera,
	renderable_iterator begin,
	renderable_iterator end)
{

	/* Maybe sort the object first */

	auto view              = camera->get_view_matrix();
	auto projection        = camera->get_projection_matrix();
	auto viewProjection    = XMMatrixMultiply(view, projection);
	auto invViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProjection), viewProjection);

	/* Setup the pipeline state */

	auto gbufferPSO = m_gbufferPST.get_permutation(device);

	commandList->SetPipelineState(gbufferPSO);

	commandList.resource_barrier_transition(gbuffer[0]->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbuffer[1]->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbuffer[2]->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.resource_barrier_transition(gbuffer[3]->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList.resource_barrier_transition(depthBuffer.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	auto dsv = depthBuffer.get_dsv_cpu_descriptor_handle();

	D3D12_CPU_DESCRIPTOR_HANDLE gbufferRTV[] = {
		gbuffer[0]->get_rtv_cpu_descriptor_handle(),
		gbuffer[1]->get_rtv_cpu_descriptor_handle(),
		gbuffer[2]->get_rtv_cpu_descriptor_handle(),
		gbuffer[3]->get_rtv_cpu_descriptor_handle()
	};
	
	commandList->OMSetRenderTargets(_countof(gbufferRTV), gbufferRTV, false, &dsv);
	commandList->OMSetStencilRef(1);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootSignature(m_gbufferRS.get());

	commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

	/* Render loop */

	material defaultMaterial;
	defaultMaterial.set_default();

	for (auto it = begin; it != end; it++)
	{

		renderable * object = *it;

		/* Fill buffer per object */

		auto world = object->get_world_matrix();;

		cb_per_object cbPerObject;

		cbPerObject.transform.world               = XMMatrixTranspose(world);
		cbPerObject.transform.worldView           = XMMatrixMultiplyTranspose(world, view);
		cbPerObject.transform.worldViewProjection = XMMatrixMultiplyTranspose(world, viewProjection);

		auto objectMaterial = object->get_material();

		material * materialData = (objectMaterial && objectMaterial->load()) ? objectMaterial.get() : &defaultMaterial;

		cbPerObject.material.baseColor = reinterpret_cast<const XMFLOAT3&>(materialData->get_base_albedo());
		cbPerObject.material.roughness = materialData->get_roughness();
		cbPerObject.material.specular  = materialData->get_specular();
		cbPerObject.material.metallic  = materialData->get_metallic();

		D3D12_GPU_VIRTUAL_ADDRESS address;

		void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_object), &address);

		if (cbData)
		{

			memcpy(cbData, &cbPerObject, sizeof(cb_per_object));

			commandList->SetGraphicsRootConstantBufferView(1, address);

			/* Draw */

			auto mesh = object->get_mesh();

			if (mesh && mesh->load())
			{

				commandList.resource_barrier_transition(mesh->get_resource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

				commandList->IASetVertexBuffers(0, 2, mesh->get_vertex_buffers());
				commandList->IASetIndexBuffer(&mesh->get_index_data());

				commandList->DrawIndexedInstanced(mesh->get_num_indices(), 1, 0, 0, 0);

			}			

		}

	}

}

void deferred_renderer::render_light(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	render_resource * const * gbuffer,
	const light * light,
	const shadow_map_info * shadowMapInfo)
{

	D3D12_GPU_VIRTUAL_ADDRESS address;

	void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_light), &address);

	if (cbData)
	{

		cb_per_light cbPerLight = {};

		cbPerLight.light.type        = light->type;
		cbPerLight.light.castShadows = shadowMapInfo ? 1u : 0u;
		cbPerLight.light.direction   = light->direction;
		cbPerLight.light.luminance   = to_float3(light->color * light->intensity);
		cbPerLight.light.ambient     = to_float3(light->ambient);
		
		cbPerLight.shadowMapping.lightMatrix = XMMatrixTranspose(shadowMapInfo->lightMatrix);

		memcpy(cbData, &cbPerLight, sizeof(cb_per_light));

		auto directionalLightingPSO = m_shadingPST.get_permutation(device, { { "LIGHT_TYPE", "LIGHT_TYPE_DIRECTIONAL" }, { "SHADOW_MAPPING_ALGORITHM", m_shadowMapAlgorithmDefine } });

		commandList->SetPipelineState(directionalLightingPSO);
		commandList->SetGraphicsRootSignature(m_shadingRS.get());

		commandList.resource_barrier_transition(gbuffer[0]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[1]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[2]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[3]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->SetGraphicsRootSignature(m_shadingRS.get());

		commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);
		commandList->SetGraphicsRootConstantBufferView(1, address);
		
		commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());
		commandList->SetGraphicsRootDescriptorTable(2, gbuffer[0]->get_srv_gpu_descriptor_handle());

		if (shadowMapInfo)
		{
			commandList.resource_barrier_transition(shadowMapInfo->shadowMap->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(3, shadowMapInfo->shadowMap->get_srv_gpu_descriptor_handle());
		}

		commandList->OMSetRenderTargets(1, &renderTarget.get_rtv_cpu_descriptor_handle(), false, nullptr);

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);

	}

}

void deferred_renderer::render_skybox(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	render_resource * const * gbuffer,
	const render_resource & depthBuffer,
	skybox & sky,
	const shadow_map_info * shadowMapInfo)
{

	D3D12_GPU_VIRTUAL_ADDRESS address;

	void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cb_per_light), &address);

	auto rtv = renderTarget.get_rtv_cpu_descriptor_handle();

	if (cbData)
	{

		light light = sky.get_sun_light();

		cb_per_light cbPerLight = {};

		cbPerLight.light.type        = light.type;
		cbPerLight.light.castShadows = shadowMapInfo ? 1u : 0u;
		cbPerLight.light.direction   = light.direction;
		cbPerLight.light.luminance   = to_float3(light.color * light.intensity);
		cbPerLight.light.ambient     = to_float3(light.ambient);

		cbPerLight.shadowMapping.lightMatrix = XMMatrixTranspose(shadowMapInfo->lightMatrix);

		memcpy(cbData, &cbPerLight, sizeof(cb_per_light));

		auto skyboxLightingPSO = m_shadingPST.get_permutation(device, { { "LIGHT_TYPE", "LIGHT_TYPE_SKYBOX" }, { "SHADOW_MAPPING_ALGORITHM", m_shadowMapAlgorithmDefine } });

		commandList->SetPipelineState(skyboxLightingPSO);
		commandList->SetGraphicsRootSignature(m_shadingRS.get());

		commandList.resource_barrier_transition(gbuffer[0]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[1]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[2]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList.resource_barrier_transition(gbuffer[3]->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);
		commandList->SetGraphicsRootConstantBufferView(1, address);

		commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());
		commandList->SetGraphicsRootDescriptorTable(2, gbuffer[0]->get_srv_gpu_descriptor_handle());

		if (shadowMapInfo)
		{

			commandList.resource_barrier_transition(shadowMapInfo->shadowMap->get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(3, shadowMapInfo->shadowMap->get_srv_gpu_descriptor_handle());

			commandList->SetGraphicsRootConstantBufferView(5, shadowMapInfo->sdsmCBAddress);

		}

		commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 1, 0, 0);

	}

	commandList->SetPipelineState(m_skyboxPSO.get());

	commandList.resource_barrier_transition(depthBuffer.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	commandList->SetGraphicsRootSignature(m_skyboxRS.get());

	commandList.resource_barrier_transition(sky.get_cubemap().get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandList->SetDescriptorHeaps(1, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_address());
	commandList->SetGraphicsRootDescriptorTable(1, sky.get_cubemap().get_srv_gpu_descriptor_handle());

	commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

	D3D12_VIEWPORT skyboxVP = m_viewport;
	skyboxVP.MinDepth = skyboxVP.MaxDepth = 1.1f;

	commandList->OMSetStencilRef(0);
	commandList->OMSetRenderTargets(1, &rtv, false, &depthBuffer.get_dsv_cpu_descriptor_handle());
	
	commandList->RSSetViewports(1, &skyboxVP);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	commandList->DrawInstanced(4, 1, 0, 0);

}

bool deferred_renderer::create_psos(ID3D12Device * device)
{
	return create_gbuffer_pso(device) &&
	       create_shading_pst(device) &&
	       create_skybox_pso(device);
}

bool deferred_renderer::create_gbuffer_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
	    !FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_gbufferRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC gbufferPSODesc = {};

		gbufferPSODesc.BlendState        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		gbufferPSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		gbufferPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		// For each pixel in the gbuffer, we set to 1 the stencil value
		// to render the skybox at the end using a stencil test.

		gbufferPSODesc.DepthStencilState.StencilEnable = TRUE;
		gbufferPSODesc.DepthStencilState.StencilWriteMask = 0x1;
		gbufferPSODesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
		gbufferPSODesc.DepthStencilState.BackFace.StencilPassOp  = D3D12_STENCIL_OP_REPLACE;

		gbufferPSODesc.NumRenderTargets      = 4;
		gbufferPSODesc.RTVFormats[0]         = m_configuration.gbufferFormat[0];
		gbufferPSODesc.RTVFormats[1]         = m_configuration.gbufferFormat[1];
		gbufferPSODesc.RTVFormats[2]         = m_configuration.gbufferFormat[2];
		gbufferPSODesc.RTVFormats[3]         = m_configuration.gbufferFormat[3];
		gbufferPSODesc.DSVFormat             = m_configuration.dsvFormat;
		gbufferPSODesc.SampleMask            = UINT_MAX;
		gbufferPSODesc.SampleDesc            = { 1, 0 };
		gbufferPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		gbufferPSODesc.pRootSignature        = m_gbufferRS.get();

		m_gbufferPST = pipeline_state_template({ },	gbufferPSODesc, "5_0");

		m_gbufferPST.set_vertex_shader("shaders/deferred_shading_gbuffer.hlsl", "gbuffer_vs");
		m_gbufferPST.set_pixel_shader("shaders/deferred_shading_gbuffer.hlsl", "gbuffer_ps");

		m_gbufferPST.set_ilv_functor(
			[](D3D12_INPUT_ELEMENT_DESC & elementDesc)
		{

			// Adjust the input desc since gpu_mesh uses slot 0 for position and
			// slot 1 for non position data (tangent space vectors, texcoords, ...)

			if (strcmp(elementDesc.SemanticName, "POSITION"))
			{
				elementDesc.InputSlot = 1;
			}

		});

		return true;

	}

	return false;

}

bool deferred_renderer::create_shading_pst(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[6];

	CD3DX12_DESCRIPTOR_RANGE gbufferSRV, shadowMapSRV, skyboxSRV;

	gbufferSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
	skyboxSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
	shadowMapSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &gbufferSRV, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsDescriptorTable(1, &shadowMapSRV, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsDescriptorTable(1, &skyboxSRV, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[5].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT),
		CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
		CD3DX12_STATIC_SAMPLER_DESC(2, D3D12_FILTER_ANISOTROPIC)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
	    !FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_shadingRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC shadingPSODesc = {};

		shadingPSODesc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		shadingPSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		shadingPSODesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
		shadingPSODesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_ONE;
		shadingPSODesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		shadingPSODesc.DepthStencilState                      = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		shadingPSODesc.DepthStencilState.DepthEnable          = FALSE;
		shadingPSODesc.NumRenderTargets                       = 1;
		shadingPSODesc.RTVFormats[0]                          = m_configuration.shadingFormat;
		shadingPSODesc.pRootSignature                         = m_shadingRS.get();
		shadingPSODesc.SampleMask                             = UINT_MAX;
		shadingPSODesc.SampleDesc                             = { 1, 0 };
		shadingPSODesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		shadingPSODesc.pRootSignature                         = m_shadingRS.get();

		m_shadingPST = pipeline_state_template(
		{
			{ "LIGHT_TYPE",{ "LIGHT_TYPE_SKYBOX", "LIGHT_TYPE_DIRECTIONAL" } },
			{ "SHADOW_MAPPING_ALGORITHM",{ "SHADOW_MAPPING_NONE", "SHADOW_MAPPING_VSM", "SHADOW_MAPPING_EVSM2", "SHADOW_MAPPING_EVSM4" } }
		},
			shadingPSODesc,
			"5_0",
			0);

		m_shadingPST.set_vertex_shader("shaders/quad_vs.hlsl", "quad_vs");
		m_shadingPST.set_pixel_shader("shaders/deferred_shading_final.hlsl", "shading_ps");

		return true;

	}

	return false;

}

bool deferred_renderer::create_skybox_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> quadVS, skyboxPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE skyboxSRV;

	skyboxSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsDescriptorTable(1, &skyboxSRV, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	D3D_SHADER_MACRO skyboxDefines[] = {
		{ "QUAD_VIEW_RAY", "" },
		{ "LIGHT_TYPE", "LIGHT_TYPE_SKYBOX" },
		{ NULL, NULL }
	};

	if (compile_shader("shaders/quad_vs.hlsl", skyboxDefines, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
		compile_shader("shaders/deferred_shading_final.hlsl", skyboxDefines, "skybox_ps", "ps_5_0", compileFlags, &skyboxPS) &&
		reflect_input_layout(quadVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_skyboxRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		// Configure the stencil to ..

		psoDesc.DepthStencilState                       = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable           = FALSE;
		psoDesc.DepthStencilState.StencilEnable         = TRUE;
		psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

		psoDesc.NumRenderTargets      = 1;
		psoDesc.RTVFormats[0]         = m_configuration.shadingFormat;
		psoDesc.DSVFormat             = m_configuration.dsvFormat;
		psoDesc.VS                    = { FUSE_BLOB_ARGS(quadVS) };
		psoDesc.PS                    = { FUSE_BLOB_ARGS(skyboxPS) };
		psoDesc.InputLayout           = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature        = m_skyboxRS.get();
		psoDesc.SampleMask            = UINT_MAX;
		psoDesc.SampleDesc            = { 1, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_skyboxPSO)));

	}

	return false;

}