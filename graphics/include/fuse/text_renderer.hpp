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
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			ID3D12Resource * rtResource,
			const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
			D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
			bitmap_font * font,
			const char * text,
			const float2 & position,
			const float4 & color);

	private:

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		bool create_pso(ID3D12Device * device);

	};

}