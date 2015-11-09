#pragma once

#include <type_traits>

#include <memory>
#include <vector>

#include <fuse/camera.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_upload_manager.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include "light.hpp"
#include "scene.hpp"
#include "renderable.hpp"

namespace fuse
{

	class deferred_renderer
	{

	public:

		deferred_renderer(void);
		deferred_renderer(const deferred_renderer &) = delete;

		bool init(ID3D12Device * device);
		void shutdown(void);

		void render_init(scene * scene);

		void render_gbuffer(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			ID3D12CommandAllocator * commandAllocator,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer * ringBuffer,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const D3D12_CPU_DESCRIPTOR_HANDLE * gbuffer,
			const D3D12_CPU_DESCRIPTOR_HANDLE * dsv,
			ID3D12Resource ** gbufferResources);

		void render_light(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			ID3D12CommandAllocator * commandAllocator,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer * ringBuffer,
			ID3D12DescriptorHeap * gbufferHeap,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			const D3D12_GPU_DESCRIPTOR_HANDLE & gbufferSRVDescTable,
			const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
			ID3D12Resource ** gbufferResources,
			const light * light);

	private:

		std::vector<renderable*> m_staticObjects;

		com_ptr<ID3D12PipelineState> m_gbufferPSO;
		com_ptr<ID3D12RootSignature> m_gbufferRS;

		com_ptr<ID3D12PipelineState> m_shadingPSO;
		com_ptr<ID3D12RootSignature> m_shadingRS;

		camera * m_camera;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT     m_scissorRect;

		static inline renderable * get_address(renderable * r) { return r; }
		static inline renderable * get_address(renderable & r) { return &r; }

		bool create_psos(ID3D12Device * device);
		bool create_gbuffer_pso(ID3D12Device * device);
		bool create_shading_pso(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(viewport, m_viewport)
			(scissor_rect, m_scissorRect)
		)

	};

}

