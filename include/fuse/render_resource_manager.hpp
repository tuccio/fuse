#pragma once

#include <fuse/render_resource.hpp>
#include <fuse/core.hpp>

#include <map>
#include <memory>
#include <vector>

#define FUSE_RENDER_RESOURCE_INVALID_ID ((UINT) (-1))

namespace fuse
{

	class render_resource_manager;

	namespace detail
	{

		typedef uint32_t render_resource_id_t;

		struct render_resource_wrapper
		{
			D3D12_RESOURCE_DESC  description;
			UINT                 bufferIndex;
			render_resource_id_t id;
			bool                 usageFlag;		
		};

		class render_resource_handle
		{

		public:

			render_resource_handle(void) : m_resource(nullptr), m_id(FUSE_RENDER_RESOURCE_INVALID_ID) {}
			render_resource_handle(const render_resource_handle &) = delete;
			render_resource_handle(render_resource_handle && r) : m_resource(nullptr), m_id(FUSE_RENDER_RESOURCE_INVALID_ID) { swap(r); }

			~render_resource_handle(void);

			render_resource_handle & operator= (const render_resource_handle & r) = delete;

			render_resource_handle & operator= (render_resource_handle && r)
			{
				swap(r);
				return *this;
			}

			void reset(void);

			inline bool operator() (void) const { return m_resource != nullptr; }
			inline void swap(render_resource_handle & r) { std::swap(m_resource, r.m_resource); std::swap(m_id, r.m_id); }

			const render_resource * operator-> (void) const { return m_resource; }
			const render_resource * get(void) const { return m_resource; }

		private:


			render_resource_handle(render_resource * r, render_resource_id_t id) :
				m_resource(r), m_id(id) {}

			render_resource_id_t m_id;
			const render_resource * m_resource;

			friend class render_resource_manager;

		};

	}

	typedef detail::render_resource_handle render_resource_ptr;

	class render_resource_manager :
		public singleton<render_resource_manager>,
		public lockable
	{

	public:

		render_resource_ptr get_texture_2d(
			ID3D12Device * device,
			UINT bufferIndex,
			DXGI_FORMAT format,
			UINT width, UINT height,
			UINT arraySize = 1u,
			UINT mipLevels = 1u,
			UINT sampleCount = 1u,
			UINT sampleQuality = 0u,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
			const D3D12_CLEAR_VALUE * clearValue = nullptr,
			const D3D12_RENDER_TARGET_VIEW_DESC * rtvDesc = nullptr,
			const D3D12_SHADER_RESOURCE_VIEW_DESC * srvDesc = nullptr,
			const D3D12_UNORDERED_ACCESS_VIEW_DESC * uavDesc = nullptr,
			const D3D12_DEPTH_STENCIL_VIEW_DESC * dsvDesc = nullptr);

		void clear(void);

	private:

		std::multimap<uint64_t, detail::render_resource_wrapper*> m_descriptionMap;

		std::vector<std::pair<render_resource*, detail::render_resource_wrapper*>> m_resources;

		detail::render_resource_id_t find_texture_2d(UINT bufferIndex, const D3D12_RESOURCE_DESC & description);
		detail::render_resource_id_t create_texture_2d(ID3D12Device * device, UINT bufferIndex, const D3D12_RESOURCE_DESC & description, const D3D12_CLEAR_VALUE * clearValue);

		// Called by render_resource_handle
		void release_texture_2d(detail::render_resource_id_t id);
		friend class detail::render_resource_handle;

	};


}