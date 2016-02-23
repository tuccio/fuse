#pragma once

#include <fuse/descriptor_heap.hpp>
#include <fuse/gpu_render_context.hpp>

#include <memory>

namespace fuse
{

	/* Scoped descriptors */

	namespace detail
	{

		template <typename DescriptorHeapType>
		class scoped_templ_descriptor
		{

		public:

			scoped_templ_descriptor(void) :
			{
				m_token = DescriptorHeapType::get_singleton_pointer()->allocate();
				m_count = 1;
			}

			scoped_templ_descriptor(uint32_t count) :
			{
				m_token = DescriptorHeapType::get_singleton_pointer()->allocate(count);
				m_count = count;
			}

			scoped_templ_descriptor(descriptor_token_t token, uint32_t count) :
				m_token(token), m_count(count) {}

			scoped_templ_descriptor(scoped_templ_descriptor &&) = delete;
			scoped_templ_descriptor(const scoped_templ_descriptor &) = delete;

			D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_descriptor_handle(void) const
			{
				return DescriptorHeapType::get_singleton_pointer()->get_cpu_descriptor_handle(m_token);
			}

			D3D12_GPU_DESCRIPTOR_HANDLE get_cpu_descriptor_handle(void) const
			{
				return DescriptorHeapType::get_singleton_pointer()->get_gpu_descriptor_handle(m_token);
			}

			~scoped_templ_descriptor(void)
			{
				gpu_render_context::get_singleton_pointer()->get_command_queue().safe_release(m_token, m_count);
			}

		private:

			descriptor_token_t m_token;
			uint32_t           m_count;

		};

	}

	typedef detail::scoped_templ_descriptor<rtv_descriptor_heap> scoped_rtv_descriptor;
	typedef detail::scoped_templ_descriptor<cbv_uav_srv_descriptor_heap> scoped_cbv_uav_srv_descriptor;
	typedef detail::scoped_templ_descriptor<dsv_descriptor_heap> scoped_dsv_descriptor;

	typedef std::shared_ptr<scoped_rtv_descriptor> shared_rtv_descriptor;
	typedef std::shared_ptr<scoped_cbv_uav_srv_descriptor> shared_cbv_uav_srv_descriptor;
	typedef std::shared_ptr<scoped_dsv_descriptor> shared_dsv_descriptor;

}
