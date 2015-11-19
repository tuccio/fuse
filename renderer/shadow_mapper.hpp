#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include "scene.hpp"

enum shadow_mapping_algorithm
{
	FUSE_SHADOW_MAPPING,
	FUSE_SHADOW_MAPPING_VSM,
	FUSE_SHADOW_MAPPING_EVSM2
};

namespace fuse
{
	
	struct shadow_mapper_configuration
	{
		DXGI_FORMAT     dsvFormat;
		DXGI_FORMAT     vsmFormat;
		DXGI_FORMAT     evsm2Format;
		D3D12_CULL_MODE cullMode;
	};

	class shadow_mapper
	{

	public:

		bool init(ID3D12Device * device, const shadow_mapper_configuration & cfg);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			ID3D12CommandAllocator * commandAllocator,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer * ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const XMMATRIX & lightMatrix,
			const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
			const D3D12_CPU_DESCRIPTOR_HANDLE * dsv,
			ID3D12Resource * renderTarget,
			ID3D12Resource * depthBuffer,
			shadow_mapping_algorithm algorithm,
			renderable_iterator begin,
			renderable_iterator end);

	private:

		shadow_mapper_configuration  m_configuration;
		
		com_ptr<ID3D12RootSignature> m_shadowMapRS;

		com_ptr<ID3D12PipelineState> m_regularShadowMapPSO;
		com_ptr<ID3D12PipelineState> m_vsmPSO;
		com_ptr<ID3D12PipelineState> m_evsm2PSO;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT     m_scissorRect;

		bool create_psos(ID3D12Device * device);
		bool create_rs(ID3D12Device * device);
		bool create_regular_pso(ID3D12Device * device);
		bool create_vsm_pso(ID3D12Device * device);
		bool create_evsm2_pso(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(viewport, m_viewport)
			(scissor_rect, m_scissorRect)
		)

	};

}