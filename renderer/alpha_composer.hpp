#pragma once

#include <fuse/directx_helper.hpp>

#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_graphics_command_list.hpp>
#include <fuse/render_resource.hpp>

#include <fuse/properties_macros.hpp>

namespace fuse
{

	struct alpha_composer_configuration
	{
		DXGI_FORMAT      rtvFormat;
		D3D12_BLEND_DESC blendDesc;
	};

	class alpha_composer
	{

	public:

		bool init(ID3D12Device * device, const alpha_composer_configuration & cfg);
		void shutdown(void);

		void render(
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			const render_resource & rtv,
			render_resource * const * shaderResources,
			uint32_t numSRVs);

	private:

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT     m_scissorRect;

		alpha_composer_configuration m_configuration;

		bool create_psos(ID3D12Device * device);

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(viewport, m_viewport)
			(scissor_rect, m_scissorRect)
		)

	};

}