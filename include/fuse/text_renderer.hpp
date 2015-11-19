#include <fuse/bitmap_font.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include <fuse/math.hpp>

namespace fuse
{

	class text_renderer
	{

	public:

		text_renderer(void) = default;
		text_renderer(const text_renderer &) = delete;

		bool init(ID3D12Device * device);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			ID3D12CommandAllocator * commandAllocator,
			gpu_graphics_command_list & commandList,
			ID3D12Resource * rtResource,
			const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			gpu_ring_buffer * ringBuffer,
			bitmap_font * font,
			const char * text,
			const XMFLOAT2 & position,
			const XMFLOAT4 & color);

	private:

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		D3D12_VIEWPORT               m_viewport;
		D3D12_RECT                   m_scissorRect;

		bool create_pso(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(viewport,     m_viewport)
			(scissor_rect, m_scissorRect)
		)

	};

}