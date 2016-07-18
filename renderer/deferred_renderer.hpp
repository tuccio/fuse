#pragma once

#include <type_traits>

#include <memory>
#include <vector>

#include <fuse/camera.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_upload_manager.hpp>
#include <fuse/core/properties_macros.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_ring_buffer.hpp>
#include <fuse/render_resource.hpp>
#include <fuse/pipeline_state.hpp>

#include "light.hpp"
#include "scene.hpp"
#include "shadow_mapping.hpp"
#include "skydome.hpp"

namespace fuse
{

	struct deferred_renderer_configuration
	{
		DXGI_FORMAT gbufferFormat[4];
		DXGI_FORMAT dsvFormat;
		DXGI_FORMAT shadingFormat;
		shadow_mapping_algorithm shadowMappingAlgorithm;
	};

	class deferred_renderer
	{

	public:

		deferred_renderer(void);
		deferred_renderer(const deferred_renderer &) = delete;

		bool init(ID3D12Device * device, const deferred_renderer_configuration & cfg);
		void shutdown(void);

		bool set_shadow_mapping_algorithm(shadow_mapping_algorithm algorithm);

		void render_gbuffer(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource * const * gbuffer,
			const render_resource & depthBuffer,
			const camera * camera,
			geometry_iterator begin,
			geometry_iterator end);

		void render_light(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource * const * gbuffer,
			const D3D12_GPU_DESCRIPTOR_HANDLE & gbufferSRVTable,
			const light * light,
			const shadow_map_info * shadowMapInfo = nullptr);

		void render_skydome(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const render_resource & renderTarget,
			const render_resource & depthBuffer,
			skydome & skydome);

	private:

		pipeline_state_template m_gbufferPST;
		pipeline_state_template m_queryPST;
		pipeline_state_template m_shadingPST;

		com_ptr<ID3D12QueryHeap> m_queryHeap;
		com_ptr<ID3D12Resource>  m_queryResult;

		com_ptr<ID3D12RootSignature> m_gbufferRS;
		com_ptr<ID3D12RootSignature> m_shadingRS;

		com_ptr<ID3D12PipelineState> m_skydomePSO;
		com_ptr<ID3D12RootSignature> m_skydomeRS;

		deferred_renderer_configuration m_configuration;
		const char * m_shadowMapAlgorithmDefine;

		bool create_psos(ID3D12Device * device);
		bool create_gbuffer_pso(ID3D12Device * device);
		bool create_shading_pst(ID3D12Device * device);
		bool create_skydome_pso(ID3D12Device * device);

	};

}

