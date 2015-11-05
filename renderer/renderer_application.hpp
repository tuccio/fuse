#pragma once

#include <fuse/application.hpp>

using namespace fuse;

struct renderer_application :
	fuse::application_base
{

	static bool on_device_created(ID3D12Device * device, ID3D12CommandQueue * commandQueue);
	static void on_device_released(ID3D12Device * device);

	static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc);

	static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam);

	static void on_render(ID3D12Device * device, gpu_command_queue & commandQueue, ID3D12Resource * backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor, UINT bufferIndex);
	static void on_update(void);

	static void on_configuration_init(fuse::application_config * configuration);

};