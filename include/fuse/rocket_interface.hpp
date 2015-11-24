#pragma once

#include <Rocket/Core.h>

#include <unordered_map>

#include <fuse/gpu_mesh.hpp>
#include <fuse/texture.hpp>

namespace fuse
{

	struct rocket_interface_configuration
	{
		D3D12_BLEND_DESC         blendDesc;
		DXGI_FORMAT              rtvFormat;
		uint32_t                 maxTextures;
	};

	class rocket_interface :
		public Rocket::Core::RenderInterface,
		public Rocket::Core::SystemInterface
	{

	public:

		rocket_interface(void);
		~rocket_interface(void);

		bool init(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_upload_manager * uploadManager, gpu_ring_buffer * ringBuffer, const rocket_interface_configuration & cfg);
		void shutdown(void);

		void render_begin(gpu_graphics_command_list & commandList, D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame, ID3D12Resource * renderTarget, const D3D12_CPU_DESCRIPTOR_HANDLE & rtv);
		void render_end(void);

		void on_resize(UINT width, UINT height);

		/* Rocket RenderInterface overrides */

		Rocket::Core::CompiledGeometryHandle CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture) override;

		void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f & translation) override;
		void ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry) override;

		void RenderGeometry(Rocket::Core::Vertex *, int, int *, int, Rocket::Core::TextureHandle, const Rocket::Core::Vector2f &) override { assert("Not implemented (shouldn't be called unless CompileGeometry fails)."); }

		void EnableScissorRegion(bool enable) override;
		void SetScissorRegion(int x, int y, int width, int height) override;

		bool LoadTexture(Rocket::Core::TextureHandle & texture_handle,
			Rocket::Core::Vector2i & texture_dimensions,
			const Rocket::Core::String & source) override;

		bool GenerateTexture(Rocket::Core::TextureHandle & texture_handle,
			const Rocket::Core::byte * source,
			const Rocket::Core::Vector2i & source_dimensions) override;

		void ReleaseTexture(Rocket::Core::TextureHandle texture_handle) override;

		/* Rocket SystemInterface overrides */

		float GetElapsedTime(void) override;

		int TranslateString(Rocket::Core::String & translated, const Rocket::Core::String & input) override;
		bool LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String & message) override;

	private:

		ID3D12Device       * m_device;
		gpu_command_queue  * m_commandQueue;
		gpu_upload_manager * m_uploadManager;
		gpu_ring_buffer    * m_ringBuffer;

		struct compiled_geometry
		{
			com_ptr<ID3D12Resource>     buffer;
			D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
			D3D12_INDEX_BUFFER_VIEW     indexBufferView;
			uint32_t                    numIndices;
			Rocket::Core::TextureHandle texture;
		};

		struct loaded_texture
		{
			texture_ptr                 texture;
			D3D12_GPU_DESCRIPTOR_HANDLE srv;
			bool                        generated;
		};

		com_ptr<ID3D12DescriptorHeap> m_srvHeap;

		std::unordered_map<Rocket::Core::CompiledGeometryHandle, compiled_geometry> m_geometry;
		std::unordered_map<Rocket::Core::TextureHandle, loaded_texture>             m_textures;

		UINT m_width;
		UINT m_height;

		bool m_scissorEnabled;

		D3D12_RECT m_scissorRect;

		gpu_graphics_command_list * m_commandList;

		D3D12_GPU_VIRTUAL_ADDRESS m_cbPerFrame;

		Rocket::Core::CompiledGeometryHandle m_lastGeometryHandle;
		Rocket::Core::TextureHandle          m_lastTextureHandle;

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		rocket_interface_configuration m_configuration;

		bool create_pso(void);

		Rocket::Core::TextureHandle register_texture(const texture_ptr & texture);

	};

}