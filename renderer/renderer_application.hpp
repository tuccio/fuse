#pragma once

#include <fuse/application.hpp>
#include <fuse/wx_windowing.hpp>
#include <fuse/scene_graph.hpp>

namespace fuse
{

	struct renderer_application :
		application_base<wx_windowing>
	{

		static bool on_render_context_created(gpu_render_context & renderContext);
		static void on_render_context_released(gpu_render_context & renderContext);

		static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc);

		static bool on_mouse_event(const mouse & mouse, const mouse_event_info & event);
		static bool on_keyboard_event(const keyboard & keyboard, const keyboard_event_info & event);

		static void on_render(gpu_render_context & renderContext, const render_resource & backBuffer);
		static void on_update(float dt);

		static void on_configuration_init(fuse::application_config * configuration);

		static bool import_scene(const char_t * filename);

		static void lock_render(void);
		static void unlock_render(void);

		static void lock_update(void);
		static void unlock_update(void);

		static void set_active_camera_node(scene_graph_camera * camera);

	private:

		static void upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, ID3D12Resource * cbPerFrameBuffer);
		static void update_renderer_configuration(ID3D12Device * device, gpu_command_queue & commandQueue);

		static void draw_gui(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList, gpu_ring_buffer & ringBuffer, const render_resource & renderTarget);

		static render_resource_ptr get_shadow_map_render_target(void);

	};

}
