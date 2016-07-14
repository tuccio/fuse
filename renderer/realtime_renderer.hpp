#pragma once

#include <fuse/gpu_render_context.hpp>
#include <fuse/scoped_descriptor.hpp>

#include "blur.hpp"
#include "scene.hpp"
#include "shadow_mapping.hpp"
#include "shadow_mapper.hpp"
#include "deferred_renderer.hpp"
#include "skydome.hpp"
#include "render_variables.hpp"

namespace fuse
{


	class realtime_renderer
	{

	public:

		bool init(gpu_render_context & renderContext, render_configuration * renderConfiguration);
		void shutdown(void);

		void release_resources(void);

		void update_render_configuration(void);
		void render(D3D12_GPU_VIRTUAL_ADDRESS cbPerFrameAddress, scene * scene, camera * camera);

		inline const render_resource * get_gbuffer_resource(int index)
		{
			return m_gbuffer[index].get();
		}

		inline const render_resource * get_final_resource(void)
		{
			return m_final.get();
		}

		inline const render_resource * get_depth_resource(void)
		{
			return m_depthBuffer.get();
		}

	private:

		gpu_render_context   * m_renderContext;
		render_configuration * m_renderConfiguration;

		uint2 m_renderResolution;

		uint32_t    m_shadowMapResolution;
		DXGI_FORMAT m_shadowMapRTV;

		render_resource_ptr m_gbuffer[4];
		render_resource_ptr m_depthBuffer;
		render_resource_ptr m_final;

		deferred_renderer m_deferredRenderer;
		shadow_mapper     m_shadowMapper;
		blur              m_shadowMapBlur;
		skydome_renderer  m_skydomeRenderer;

		visual_debugger * m_visualDebugger;

		renderable_vector m_renderables;

		std::vector<scoped_cbv_uav_srv_descriptor> m_gbufferSRVTable;

		render_resource_ptr get_shadow_map_render_target(void);
		bool setup_shadow_mapping(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_VALUE(
			(visual_debugger, m_visualDebugger)
		)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(culling_results, m_renderables)
		)

	};

}