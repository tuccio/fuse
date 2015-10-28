#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>

#include <fuse/singleton.hpp>
#include <fuse/directx_helper.hpp>

#include <queue>

#define FUSE_FPS_SAMPLES  16
#define FUSE_WINDOW_CLASS "fuse_window_class"

namespace fuse
{

	struct application_config
	{
		
		UINT syncInterval;
		UINT presentFlags;

		DXGI_FORMAT      swapChainFormat;
		DXGI_SAMPLE_DESC sampleDesc;
		DXGI_RATIONAL    refreshRate;

		UINT       swapChainBufferCount;
		DXGI_USAGE swapChainBufferUsage;

	};

	struct application_base
	{

	public:

		typedef application_base base_type;

		static bool init(HINSTANCE hInstance, bool silent = false);
		static void shutdown(void);

		static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		inline static void on_configuration_init(application_config * configuration) { }

		inline static bool on_device_created(ID3D12Device * device, ID3D12CommandQueue * commandQueue) { return true; }
		inline static void on_device_released(ID3D12Device * device) { }

		inline static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc) { return true; }
		inline static void on_swap_chain_released(ID3D12Device * device, IDXGISwapChain * swapChain) { }

		inline static void on_update(void) { }
		inline static void on_render(ID3D12Device * device, ID3D12CommandQueue * commandQueue, D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor) { }

		static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam);

		static float get_fps(void);

		static void set_fullscreen(bool fullscreen);
		static bool set_cursor(bool clippedFullscreen, bool hidden);

		static bool create_window(int width, int height, const char * caption);
		static void destroy_window(void);

		inline static HWND get_window(void) { return m_hWnd; }
								
		inline static int  get_screen_width(void) { return m_screenWidth; }
		inline static int  get_screen_height(void) { return m_screenHeight; }
						   
		inline static HINSTANCE get_instance(void) { return m_hInstance; }
		
		inline static ID3D12Device       * get_device(void) { return m_device.get(); }
		inline static ID3D12CommandQueue * get_command_queue(void) { return m_commandQueue.get(); }
		inline static IDXGISwapChain     * get_swap_chain(void) { return m_swapChain.get(); }
		
		inline static void set_screen_viewport(ID3D12GraphicsCommandList * cmdList)
		{

			D3D12_VIEWPORT viewport = { 0.f, 0.f, (float) m_swapChainBufferDesc.Width, (float) m_swapChainBufferDesc.Height, 0.f, 1.f };
			D3D12_RECT     rect     = { 0, 0, (LONG) m_swapChainBufferDesc.Width, (LONG) m_swapChainBufferDesc.Height };

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &rect);

		}

		inline static ID3D12Resource * get_back_buffer(void) { return m_renderTargets[m_bufferIndex].get(); }

		inline static D3D12_CPU_DESCRIPTOR_HANDLE get_back_buffer_descriptor(void)
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(
				m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
				m_bufferIndex,
				m_rtvDescriptorSize);
		}

	protected:

		static bool                                 m_initialized;
		static bool                                 m_silent;
		static bool                                 m_debug;

		static HINSTANCE                            m_hInstance;
		static HWND                                 m_hWnd;

		static bool                                 m_resizeSwapChain;

		static int                                  m_screenWidth;
		static int                                  m_screenHeight;

		static com_ptr<ID3D12Device>                m_device;
		static com_ptr<IDXGISwapChain3>             m_swapChain;

		static com_ptr<ID3D12CommandQueue>          m_commandQueue;

		static com_ptr<ID3D12DescriptorHeap>        m_rtvHeap;
		static int                                  m_rtvDescriptorSize;

		static std::vector<com_ptr<ID3D12Resource>> m_renderTargets;

		static com_ptr<ID3D12Fence>                 m_fence;
		static UINT64                               m_fenceValue;
		static HANDLE                               m_fenceEvent;

		static DXGI_SURFACE_DESC                    m_swapChainBufferDesc;
		static application_config                   m_configuration;

		static int                                  m_bufferIndex;

		static float                                m_frameSamples[FUSE_FPS_SAMPLES];

		/* Functions called by d3d12_windows_application */

		static void set_default_configuration(void);

		static bool create_device(D3D_FEATURE_LEVEL featureLevel, bool debug);
		static bool create_command_queue(void);
		static bool create_swap_chain(bool debug, int width, int height);
		static bool resize_swap_chain(int width, int height);

		static bool create_render_target_views(void);
		static bool update_render_target_views(void);
		static void release_render_target_views(void);

		static bool get_swap_chain_buffers(void);
		static void release_swap_chain_buffers(void);

		static const DXGI_SURFACE_DESC * get_swap_chain_buffer_desc(void) { return &m_swapChainBufferDesc; }

		static void signal_error(const char * error);

		inline static void update_fps_counter(void)
		{

			static LARGE_INTEGER oldTime;
			static int           frameIndex;
			static float         frequency;

			static struct __init_frequency
			{
				__init_frequency(void)
				{
					LARGE_INTEGER iFrequency;
					QueryPerformanceFrequency(&iFrequency);
					frequency = static_cast<float>(iFrequency.QuadPart);
				}
			} frequencyInitializer;

			LARGE_INTEGER newTime;
			QueryPerformanceCounter(&newTime);

			m_frameSamples[frameIndex] = frequency / static_cast<float>(newTime.QuadPart - oldTime.QuadPart);

			frameIndex = (frameIndex + 1) % FUSE_FPS_SAMPLES;
			oldTime = newTime;

		}

	};

	template <typename ConcreteApplication>
	struct application :
		ConcreteApplication
	{

	public:

		static inline bool init(HINSTANCE hInstance)
		{

			if (base_type::init(hInstance))
			{

				SetWindowsHook(WH_KEYBOARD, on_keyboard);
				SetWindowsHook(WH_MOUSE, on_mouse);

				return true;

			}

			return false;

		}

		static inline bool create_pipeline(D3D_FEATURE_LEVEL featureLevel, bool debug)
		{

			set_default_configuration();
			on_configuration_init(&m_configuration);

			bool success = create_device(featureLevel, debug) &&
				create_command_queue() &&
				on_device_created(get_device(), get_command_queue()) &&
				create_swap_chain(debug, get_screen_width(), get_screen_height());

			if (!success) signal_error("Failed to create graphics pipeline.");

			return success;

		}

		static inline void shutdown(void)
		{
			
			if (m_swapChain)
			{
				on_swap_chain_released(get_device(), get_swap_chain());
				release_swap_chain_buffers();
				release_render_target_views();
			}

			if (m_device)
			{	
				on_device_released(get_device());
			}

			base_type::shutdown();
			
		}

		static inline void main_loop(void)
		{

			MSG msg;

			do
			{

				if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				wait_for_last_frame();
				update_fps_counter();

				on_update();
				
				if (!update_swapchain())
				{
					signal_error("Failed to resize the swap chain.");
					return;
				}

				on_render(get_device(), get_command_queue(), get_back_buffer(), get_back_buffer_descriptor());

				m_swapChain->Present(m_configuration.syncInterval, m_configuration.presentFlags);

			} while (msg.message != WM_QUIT);

		}

		static inline void wait_for_last_frame(void)
		{
			
			/* Wait for the last frame to complete */

			UINT64 fenceValue = m_fenceValue++;

			FUSE_HR_CHECK(m_commandQueue->Signal(m_fence.get(), fenceValue));

			if (m_fence->GetCompletedValue() < fenceValue)
			{
				FUSE_HR_CHECK(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
				WaitForSingleObject(m_fenceEvent, INFINITE);
			}

		}

		static inline bool update_swapchain(void)
		{

			// Only resize if m_resizeSwapChain == true

			bool success = !m_resizeSwapChain ||
			               (resize_swap_chain(get_screen_width(), get_screen_height()) &&
			               on_swap_chain_resized(get_device(), get_swap_chain(), get_swap_chain_buffer_desc()));
			
			m_resizeSwapChain = false;
			m_bufferIndex     = m_swapChain->GetCurrentBackBufferIndex();

			return success;

		}

	};

}