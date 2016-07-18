#include "realtime_renderer.hpp"
#include "render_variables.hpp"

#include <sstream>

using namespace fuse;

bool realtime_renderer::init(gpu_render_context & renderContext, render_configuration * renderConfiguration)
{
	m_renderContext       = &renderContext;
	m_renderConfiguration = renderConfiguration;

	ID3D12Device * device = m_renderContext->get_device();

	deferred_renderer_configuration rendererCFG;

	rendererCFG.gbufferFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.gbufferFormat[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rendererCFG.gbufferFormat[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rendererCFG.dsvFormat        = DXGI_FORMAT_D24_UNORM_S8_UINT;
	rendererCFG.shadingFormat    = DXGI_FORMAT_R16G16B16A16_FLOAT;

	rendererCFG.shadowMappingAlgorithm = m_renderConfiguration->get_shadow_mapping_algorithm();

	debug_renderer_configuration debugRendererCFG;

	debugRendererCFG.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	debugRendererCFG.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	for (int i = 0; i < m_renderContext->get_buffers_count(); i++)
	{
		// Create a new SRV table with 4 entries (for the gbuffer)
		m_gbufferSRVTable.emplace_back(4);
	}

	m_renderResolution    = m_renderConfiguration->get_render_resolution();
	m_shadowMapResolution = m_renderConfiguration->get_shadow_map_resolution();

	return m_deferredRenderer.init(device, rendererCFG) &&
		m_skydomeRenderer.init(device) &&
		setup_shadow_mapping(device);
}

void realtime_renderer::shutdown(void)
{
	release_resources();

	m_deferredRenderer.shutdown();
	m_skydomeRenderer.shutdown();

	m_gbufferSRVTable.clear();
}

void realtime_renderer::release_resources(void)
{
	m_gbuffer[0].reset();
	m_gbuffer[1].reset();
	m_gbuffer[2].reset();
	m_gbuffer[3].reset();

	m_final.reset();

	m_depthBuffer.reset();
}

void realtime_renderer::render(D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress, scene * scene, camera * camera)
{
	release_resources();

	ID3D12Device * device = m_renderContext->get_device();

	uint32_t bufferIndex = m_renderContext->get_buffer_index();

	auto & commandQueue = m_renderContext->get_command_queue();
	auto & commandList  = m_renderContext->get_command_list();

	/* Setup render */

	render_resource_manager & renderResourceManager = render_resource_manager::get_singleton_reference();

	/* Get the resources for gbuffer rendering and clear them */

	m_gbuffer[0] = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, color_rgba::zero));

	m_gbuffer[1] = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, color_rgba::zero));

	m_gbuffer[2] = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, color_rgba::zero));

	m_gbuffer[3] = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, color_rgba::zero));

	for (int i = 0; i < 4; i++)
	{

		stringstream_t ss;

		ss << "gbuffer" << i <<  " bufferIndex: " << bufferIndex;

		m_gbuffer[i]->get()->SetName(ss.str().c_str());

		device->CreateShaderResourceView(
			m_gbuffer[i]->get(),
			nullptr,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(
				m_gbufferSRVTable[bufferIndex].get_cpu_descriptor_handle(),
				i,
				m_gbufferSRVTable[bufferIndex].get_descriptor_size()));

	}

	const render_resource * gbuffer[] = {
		m_gbuffer[0].get(),
		m_gbuffer[1].get(),
		m_gbuffer[2].get(),
		m_gbuffer[3].get()
	};

	m_depthBuffer = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_D24_UNORM_S8_UINT,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0));
	//auto dsvHandle = g_depthBuffer[bufferIndex].get_dsv_cpu_descriptor_handle();

	auto dsvHandle = m_depthBuffer->get_dsv_cpu_descriptor_handle();

	commandList->ClearRenderTargetView(gbuffer[0]->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[1]->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[2]->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[3]->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);

	commandList->ClearDepthStencilView(m_depthBuffer->get_dsv_cpu_descriptor_handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

	/* GBuffer rendering */

	m_renderedGeometry = scene->frustum_culling(camera->get_frustum());

	//FUSE_LOG(FUSE_LITERAL("realtime_renderer"), stringstream_t() << "Drawing " << m_renderables.size() << " objects.");

	auto geometry = std::make_pair(m_renderedGeometry.begin(), m_renderedGeometry.end());
	//auto renderableObjects = scene->get_geometry();

	auto fullscreenViewport    = make_fullscreen_viewport(m_renderResolution.x, m_renderResolution.y);
	auto fullscreenScissorRect = make_fullscreen_scissor_rect(m_renderResolution.x, m_renderResolution.y);

	commandList->RSSetViewports(1, &fullscreenViewport);
	commandList->RSSetScissorRects(1, &fullscreenScissorRect);

	m_deferredRenderer.render_gbuffer(
		device,
		commandQueue,
		commandList,
		m_renderContext->get_ring_buffer(),
		cbPerFrameAddress,
		gbuffer,
		*m_depthBuffer.get(),
		camera,
		geometry.first,
		geometry.second);

	//g_visualDebugger.add(device, commandList, bufferIndex, *gbufferResources[0].get(), XMUINT2(16, 16), g_visualDebugger.get_textures_scale());

	/* Prepare for shading */

	m_final = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R16G16B16A16_FLOAT,
		m_renderResolution.x, m_renderResolution.y, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, color_rgba::zero));

	commandList.resource_barrier_transition(m_final->get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ClearRenderTargetView(m_final->get_rtv_cpu_descriptor_handle(), color_rgba::zero, 0, nullptr);

	/* Shading */

	/*auto lightsIterators = g_scene.get_lights();

	for (auto it = lightsIterators.first; it != lightsIterators.second; it++)
	{

	light * currentLight = *it;

	shadow_map_info shadowMapInfo, * pShadowMapInfo = nullptr;

	if (currentLight->type == FUSE_LIGHT_TYPE_DIRECTIONAL)
	{

	shadowMapInfo.shadowMap   = std::addressof(g_shadowmap0[bufferIndex]);
	shadowMapInfo.lightMatrix = sm_directional_light_matrix(currentLight->direction, staticObjects.first, staticObjects.second);

	g_shadowMapper.render(
	device,
	commandQueue,
	commandList,
	renderContext.get_ring_buffer(),
	cbPerFrameAddress,
	shadowMapInfo.lightMatrix,
	g_shadowmap0[bufferIndex],
	g_shadowmap32F[bufferIndex],
	staticObjects.first,
	staticObjects.second);

	g_shadowMapBlur.render(
	commandQueue,
	commandList,
	g_shadowmap0[bufferIndex],
	g_shadowmap1[bufferIndex]);

	g_mipmapGenerator->generate_mipmaps(device, commandQueue, commandList, g_shadowmap0[bufferIndex].get());

	pShadowMapInfo = &shadowMapInfo;

	}

	g_deferredRenderer.render_light(
	device,
	commandQueue,
	commandList,
	renderContext.get_ring_buffer(),
	cbPerFrameAddress,
	g_fullresRGBA16F2[bufferIndex],
	gbuffer,
	currentLight,
	pShadowMapInfo);

	}*/

	/* Skybox and skylight */

	render_resource_ptr shadowMapResources[] = {
		get_shadow_map_render_target(),
		get_shadow_map_render_target()
	};

	D3D12_DEPTH_STENCIL_VIEW_DESC shadowMapDSVDesc = {};

	shadowMapDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowMapDSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	render_resource_ptr shadowMapDepth = renderResourceManager.get_texture_2d(
		device, bufferIndex, DXGI_FORMAT_R32_TYPELESS,
		m_shadowMapResolution, m_shadowMapResolution, 1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.f, 0),
		nullptr, nullptr, nullptr,
		&shadowMapDSVDesc);

	{
		shadow_map_info shadowMapInfo = {};

		//shadowMapInfo.sdsm          = true;
		//shadowMapInfo.sdsmCBAddress = g_sdsmConstantBuffer[bufferIndex]->GetGPUVirtualAddress();

		skydome * skydome = scene->get_skydome();

		m_skydomeRenderer.render(device, commandQueue, commandList, m_renderContext->get_ring_buffer(), *skydome);

		if (m_visualDebugger && m_visualDebugger->get_draw_skydome())
		{
			m_visualDebugger->add(device, commandList, bufferIndex, skydome->get_current_skydome(), uint2(16, 16), m_visualDebugger->get_textures_scale(), true);
		}

		light sunLight = skydome->get_sun_light();

		shadowMapInfo.shadowMap = shadowMapResources[0].get();

		{
			mat128 lightViewMatrix = look_at_lh(vec128_zero(), to_vec128(sunLight.direction), to_vec128(camera->left()));
			//vec128_set(sunLight.direction.y > .99f ? 1.f : 0.f, sunLight.direction.y > .99f ? 0.f : 1.f, 0.f, 0.f));

			mat128 lightCropMatrix = sm_crop_matrix_lh(lightViewMatrix, geometry.first, geometry.second);

			shadowMapInfo.lightMatrix = lightViewMatrix * lightCropMatrix;
		}

		/* Sun shadow map */

		m_shadowMapper.render(
			device,
			commandQueue,
			commandList,
			m_renderContext->get_ring_buffer(),
			cbPerFrameAddress,
			shadowMapInfo.lightMatrix,
			*shadowMapResources[0].get(),
			*shadowMapDepth.get(),
			geometry.first,
			geometry.second);

		m_shadowMapBlur.render(
			commandQueue,
			commandList,
			*shadowMapResources[0].get(),
			*shadowMapResources[1].get());

		mipmap_generator::get_singleton_pointer()->generate_mipmaps(device, commandQueue, commandList, shadowMapResources[0]->get());

		/* Skylight rendering */

		commandList->RSSetViewports(1, &fullscreenViewport);
		commandList->RSSetScissorRects(1, &fullscreenScissorRect);

		m_deferredRenderer.render_light(
			device,
			commandQueue,
			commandList,
			m_renderContext->get_ring_buffer(),
			cbPerFrameAddress,
			*m_final.get(),
			gbuffer,
			m_gbufferSRVTable[bufferIndex].get_gpu_descriptor_handle(),
			&sunLight,
			&shadowMapInfo);

		/* Skybox rendering */

		m_deferredRenderer.render_skydome(
			device,
			commandQueue,
			commandList,
			cbPerFrameAddress,
			*m_final.get(),
			*m_depthBuffer.get(),
			*skydome);
	}
}

render_resource_ptr realtime_renderer::get_shadow_map_render_target(void)
{
	return render_resource_manager::get_singleton_pointer()->get_texture_2d(
		m_renderContext->get_device(),
		m_renderContext->get_buffer_index(),
		m_shadowMapRTV,
		m_shadowMapResolution,
		m_shadowMapResolution,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		&CD3DX12_CLEAR_VALUE(m_shadowMapRTV, color_rgba::zero));
}

#define FAIL_IF(Condition) if (Condition) return false;

bool realtime_renderer::setup_shadow_mapping(ID3D12Device * device)
{
	uint32_t resolution = m_renderConfiguration->get_shadow_map_resolution();
	shadow_mapping_algorithm algorithm = m_renderConfiguration->get_shadow_mapping_algorithm();

	switch (algorithm)
	{

	case FUSE_SHADOW_MAPPING_NONE:
		return true;

	case FUSE_SHADOW_MAPPING_VSM:

		if (m_renderConfiguration->get_evsm2_float_precision() == 16)
		{
			m_shadowMapRTV = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			m_shadowMapRTV = DXGI_FORMAT_R32G32_FLOAT;
		}

		FAIL_IF(!m_shadowMapBlur.init_box_blur(
			device,
			"float2",
			m_renderConfiguration->get_vsm_blur_kernel_size(),
			resolution, resolution));

			break;

	case FUSE_SHADOW_MAPPING_EVSM2:

		if (m_renderConfiguration->get_evsm2_float_precision() == 16)
		{
			m_shadowMapRTV = DXGI_FORMAT_R16G16_FLOAT;
		}
		else
		{
			m_shadowMapRTV = DXGI_FORMAT_R32G32_FLOAT;
		}

		FAIL_IF(!m_shadowMapBlur.init_box_blur(
			device,
			"float2",
			m_renderConfiguration->get_evsm2_blur_kernel_size(),
			resolution, resolution))

			break;

	case FUSE_SHADOW_MAPPING_EVSM4:

		if (m_renderConfiguration->get_evsm4_float_precision() == 16)
		{
			m_shadowMapRTV = DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		else
		{
			m_shadowMapRTV = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		FAIL_IF(!m_shadowMapBlur.init_box_blur(
			device,
			"float4",
			m_renderConfiguration->get_evsm4_blur_kernel_size(),
			resolution, resolution))

			break;

	}

	shadow_mapper_configuration shadowMapperCFG = {};

	shadowMapperCFG.cullMode  = D3D12_CULL_MODE_NONE;
	shadowMapperCFG.rtvFormat = m_shadowMapRTV;
	shadowMapperCFG.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	shadowMapperCFG.algorithm = algorithm;

	m_shadowMapper.shutdown();
	m_shadowMapper.init(device, shadowMapperCFG);

	m_shadowMapper.set_viewport(make_fullscreen_viewport(resolution, resolution));
	m_shadowMapper.set_scissor_rect(make_fullscreen_scissor_rect(resolution, resolution));

	render_resource_manager::get_singleton_pointer()->clear();

	return true;
}

#undef FAIL_IF

void realtime_renderer::update_render_configuration(void)
{
	auto updates = m_renderConfiguration->get_updates();

	ID3D12Device      * device       = m_renderContext->get_device();
	gpu_command_queue & commandQueue = m_renderContext->get_command_queue();

	for (auto variable : updates)
	{
		switch (variable)
		{

		case FUSE_RVAR_RENDER_RESOLUTION:
			commandQueue.wait_for_frame(commandQueue.get_frame_index());
			release_resources();
			render_resource_manager::get_singleton_pointer()->clear();
			m_renderResolution = m_renderConfiguration->get_render_resolution();
			break;

		case FUSE_RVAR_VSM_FLOAT_PRECISION:
		case FUSE_RVAR_EVSM2_FLOAT_PRECISION:
		case FUSE_RVAR_EVSM4_FLOAT_PRECISION:
		case FUSE_RVAR_SHADOW_MAPPING_ALGORITHM:
			commandQueue.wait_for_frame(commandQueue.get_frame_index());
			setup_shadow_mapping(device);
			m_deferredRenderer.set_shadow_mapping_algorithm(m_renderConfiguration->get_shadow_mapping_algorithm());
			break;

		case FUSE_RVAR_SHADOW_MAP_RESOLUTION:
			commandQueue.wait_for_frame(commandQueue.get_frame_index());
			m_shadowMapResolution = m_renderConfiguration->get_shadow_map_resolution();
			setup_shadow_mapping(device);
			break;

		case FUSE_RVAR_VSM_BLUR_KERNEL_SIZE:
		case FUSE_RVAR_EVSM2_BLUR_KERNEL_SIZE:
		case FUSE_RVAR_EVSM4_BLUR_KERNEL_SIZE:
			setup_shadow_mapping(device);
			break;

		}
	}
}
