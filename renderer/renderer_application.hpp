#pragma once

#include <fuse/application.hpp>

namespace fuse
{

	struct renderer_application :
		application_base
	{

		static bool on_device_created(ID3D12Device * device, gpu_command_queue & commandQueue);
		static void on_device_released(ID3D12Device * device);

		static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc);

		static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam);

		static LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		static void on_render(ID3D12Device * device, gpu_command_queue & commandQueue, const render_resource & backBuffer, UINT bufferIndex);
		static void on_update(float dt);

		static void on_configuration_init(fuse::application_config * configuration);

		static void upload_per_frame_resources(ID3D12Device * device, gpu_command_queue & commandQueue, ID3D12Resource * cbPerFrameBuffer);
		static bool create_shadow_map_resources(ID3D12Device * device, bool createBuffers = true);

		static void update_renderer_configuration(ID3D12Device * device, gpu_command_queue & commandQueue);

		static void draw_gui(ID3D12Device * device, gpu_command_queue & commandQueue, gpu_graphics_command_list & commandList);

	};

}
