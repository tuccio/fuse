#pragma once

#include <type_traits>

#include <memory>
#include <vector>

#include <fuse/camera.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_upload_manager.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include "light.hpp"
#include "scene.hpp"
#include "renderable.hpp"

class deferred_renderer
{

public:

	deferred_renderer(void);
	deferred_renderer(const deferred_renderer &) = delete;

	bool init(ID3D12Device * device, ID3D12Resource * cbPerFrame, fuse::gpu_upload_manager * uploadManager);
	void shutdown(void);

	void on_resize(uint32_t width, uint32_t height);

	void render_init(scene * scene);

	void render_gbuffer(
		ID3D12Device * device,
		fuse::gpu_command_queue & commandQueue,
		ID3D12CommandAllocator * commandAllocator,
		ID3D12GraphicsCommandList * commandList,
		fuse::gpu_ring_buffer & ringBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE * gbuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE * dsv);

	void render_directional_light(
		ID3D12Device * device,
		ID3D12GraphicsCommandList * commandList,
		const D3D12_GPU_VIRTUAL_ADDRESS * gbuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
		const light * light);

private:
	
	std::vector<renderable*> m_staticObjects;

	fuse::com_ptr<ID3D12Resource> m_cbPerFrame;

	fuse::com_ptr<ID3D12Fence> m_cbFence;
	HANDLE                     m_hFenceEvent;

	fuse::com_ptr<ID3D12PipelineState> m_gbufferPSO;
	fuse::com_ptr<ID3D12RootSignature> m_gbufferRS;

	fuse::com_ptr<ID3D12PipelineState> m_finalPSO;
	fuse::com_ptr<ID3D12RootSignature> m_finalRS;

	D3D12_GPU_VIRTUAL_ADDRESS m_cbPerFrameAddress;

	fuse::gpu_upload_manager * m_uploadManager;

	bool m_debug;

	fuse::camera * m_camera;

	uint32_t m_width;
	uint32_t m_height;

	/*uint32_t m_buffers;
	uint32_t m_bufferIndex;*/

	static inline renderable * get_address(renderable * r) { return r; }
	static inline renderable * get_address(renderable & r) { return &r; }

	bool create_psos(ID3D12Device * device);
	bool create_gbuffer_pso(ID3D12Device * device);
	bool create_final_pso(ID3D12Device * device);

public:

	FUSE_PROPERTIES_BY_VALUE(
		(debug, m_debug)
	)

};