#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/properties_macros.hpp>
#include <fuse/math.hpp>
#include <fuse/color.hpp>
#include <fuse/descriptor_heap.hpp>
#include <fuse/render_resource.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/gpu_ring_buffer.hpp>

#include "light.hpp"

namespace fuse
{

	class skybox_renderer;
	
	class skybox
	{

	public:

		skybox(void) :
			m_turbidity(2.f),
			m_zenith(0.f),
			m_azimuth(0.f),
			m_lastUpdatedBuffer(0) { }

		skybox(const skybox &) = delete;
		skybox(skybox &&) = default;

		~skybox(void) { shutdown(); }

		skybox & operator= (skybox &&) = default;

		bool init(ID3D12Device * device, uint32_t resolution = 128u, uint32_t buffers = 1);
		void shutdown(void);

		render_resource & get_cubemap(void);

		light get_sun_light(void) const;

	private:

		uint32_t  m_resolution;

		float     m_turbidity;
		color_rgb m_groundAlbedo;

		float     m_zenith;
		float     m_azimuth;

		bool      m_uptodate;

		std::vector<render_resource> m_cubemaps;
		uint32_t m_lastUpdatedBuffer;

		friend class skybox_renderer;

	public:

		FUSE_PROPERTIES_BY_VALUE_READ_ONLY (
			(turbidity, m_turbidity)
			(zenith, m_zenith)
			(azimuth, m_azimuth)
			(resolution, m_resolution)
			(uptodate, m_uptodate)
		)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY (
			(ground_abledo, m_groundAlbedo)
		)

		inline void set_turbidity(float turbidity) { m_turbidity = turbidity; m_uptodate = false; }
		inline void set_zenith(float zenith) { m_zenith = zenith; m_uptodate = false; }
		inline void set_azimuth(float azimuth) { m_azimuth = azimuth; m_uptodate = false; }
		inline void set_ground_albedo(const color_rgb & groundAlbedo) { m_groundAlbedo = groundAlbedo; m_uptodate = false; }
	};

	class skybox_renderer
	{

	public:

		bool init(ID3D12Device * device);
		void shutdown(void);

		void render(
			ID3D12Device * device,
			gpu_command_queue & commandQueue,
			gpu_graphics_command_list & commandList,
			gpu_ring_buffer & ringBuffer,
			skybox & skybox);

	private:

		com_ptr<ID3D12PipelineState> m_pso;
		com_ptr<ID3D12RootSignature> m_rs;

		bool create_pso(ID3D12Device * device);

	};

}