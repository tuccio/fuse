#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include <fuse/render_resource.hpp>

#include "scene.hpp"
#include "shadow_mapping.hpp"

namespace fuse
{
	
	struct shadow_mapper_configuration
	{
		shadow_mapping_algorithm algorithm;
		DXGI_FORMAT              rtvFormat;
		DXGI_FORMAT              dsvFormat;
		D3D12_CULL_MODE          cullMode;
	};

	class shadow_mapper
	{

	public:

		bool init(ID3D12Device * device, const shadow_mapper_configuration & cfg);
		void shutdown(void);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer * ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const XMMATRIX & lightMatrix,
			const render_resource & renderTarget,
			const render_resource & depthBuffer,
			renderable_iterator begin,
			renderable_iterator end);

	private:

		shadow_mapper_configuration  m_configuration;
		
		com_ptr<ID3D12RootSignature> m_rs;
		com_ptr<ID3D12PipelineState> m_pso;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT     m_scissorRect;

		bool create_psos(ID3D12Device * device);
		bool create_rs(ID3D12Device * device);
		bool create_regular_pso(ID3D12Device * device);
		bool create_vsm_pso(ID3D12Device * device);
		bool create_evsm2_pso(ID3D12Device * device);
		bool create_evsm4_pso(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(viewport, m_viewport)
			(scissor_rect, m_scissorRect)
		)

	};

}