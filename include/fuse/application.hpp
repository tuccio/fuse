#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dx12.h>

#include <fuse/singleton.hpp>
#include <fuse/gpu_command_queue.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/gpu_global_resource_state.hpp>
#include <fuse/render_resource.hpp>
#include <fuse/descriptor_heap.hpp>
#include <fuse/gpu_render_context.hpp>

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

		UINT                 swapChainBufferCount;
		DXGI_USAGE           swapChainBufferUsage;
		DXGI_SWAP_EFFECT     swapChainSwapEffect;
		DXGI_SWAP_CHAIN_FLAG swapChainFlags;

		UINT maxDSV;
		UINT maxCBVUAVSRV;
		UINT maxRTV;

		UINT uploadHeapSize;

	};

	struct application_base
	{

	public:

		typedef application_base base_type;

		static bool init(HINSTANCE hInstance, bool silent = false);
		static void shutdown(void);

		static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		inline static void on_configuration_init(application_config * configuration) { }

		inline static bool on_render_context_created(gpu_render_context & renderContext) { return true; }
		inline static void on_render_context_released(gpu_render_context & renderContext) { }

		inline static bool on_swap_chain_resized(ID3D12Device * device, IDXGISwapChain * swapChain, const DXGI_SURFACE_DESC * desc) { return true; }
		inline static void on_swap_chain_released(ID3D12Device * device, IDXGISwapChain * swapChain) { }

		inline static void on_update(float dt) { }
		inline static void on_render(gpu_render_context & renderContext, const render_resource & backBuffer) { }
		inline static LRESULT on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }

		static LRESULT CALLBACK on_keyboard(int code, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK on_mouse(int code, WPARAM wParam, LPARAM lParam);

		static float get_fps(void);

		static bool is_fullscreen(void);
		static void set_fullscreen(bool fullscreen);
		static bool set_cursor(bool clippedFullscreen, bool hidden);

		static bool create_descriptor_heaps(void);
		static void destroy_descriptor_heaps(void);

		static bool create_window(int width, int height, const char * caption);
		static void destroy_window(void);

		inline static HWND get_window(void) { return m_hWnd; }
								
		inline static int get_screen_width(void) { return m_screenWidth; }
		inline static int get_screen_height(void) { return m_screenHeight; }
						   
		inline static HINSTANCE get_instance(void) { return m_hInstance; }
		
		inline static ID3D12Device      * get_device(void) { return m_device.get(); }
		inline static IDXGISwapChain    * get_swap_chain(void) { return m_swapChain.get(); }
		inline static gpu_command_queue & get_command_queue(void) { return m_renderContext.get_command_queue(); }

		inline static void set_screen_viewport(ID3D12GraphicsCommandList * cmdList)
		{

			D3D12_VIEWPORT viewport = { 0.f, 0.f, (float) m_swapChainBufferDesc.Width, (float) m_swapChainBufferDesc.Height, 0.f, 1.f };
			D3D12_RECT     rect     = { 0, 0, (LONG) m_swapChainBufferDesc.Width, (LONG) m_swapChainBufferDesc.Height };

			cmdList->RSSetViewports(1, &viewport);
			cmdList->RSSetScissorRects(1, &rect);

		}

		inline static render_resource & get_back_buffer(void) { return m_renderTargets[m_bufferIndex]; }
		inline static UINT              get_buffer_index(void) { return m_bufferIndex; }

	protected:

		static bool                          m_initialized;
		static bool                          m_silent;
		static bool                          m_debug;

		static HINSTANCE                     m_hInstance;
		static HWND                          m_hWnd;

		static bool                          m_resizeSwapChain;

		static int                           m_screenWidth;
		static int                           m_screenHeight;

		static com_ptr<ID3D12Device>         m_device;
		static com_ptr<IDXGISwapChain3>      m_swapChain;

		static cbv_uav_srv_descriptor_heap   m_shaderDescriptorHeap;
		static rtv_descriptor_heap           m_renderTargetDescriptorHeap;
		static dsv_descriptor_heap           m_depthStencilDescriptorHeap;

		static std::vector<render_resource>  m_renderTargets;

		static DXGI_SURFACE_DESC             m_swapChainBufferDesc;
		static application_config            m_configuration;

		static UINT                          m_bufferIndex;

		static gpu_render_context            m_renderContext;

		static float                         m_frameSamples[FUSE_FPS_SAMPLES];
		static gpu_global_resource_state     m_globalState;

		/* Functions called by d3d12_windows_application */

		static void set_default_configuration(void);

		static bool create_device(D3D_FEATURE_LEVEL featureLevel, bool debug);
		static bool create_render_context(void);

		static bool create_swap_chain(bool debug, int width, int height);
		static bool resize_swap_chain(int width, int height);

		static bool get_swap_chain_buffers(void);
		static void release_swap_chain_buffers(void);

		static const DXGI_SURFACE_DESC * get_swap_chain_buffer_desc(void) { return &m_swapChainBufferDesc; }

		static void signal_error(const char * error);

		inline static float update_fps_counter(void)
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

			float dt = static_cast<float>(newTime.QuadPart - oldTime.QuadPart);

			m_frameSamples[frameIndex] = frequency / dt;

			frameIndex = (frameIndex + 1) % FUSE_FPS_SAMPLES;
			oldTime    = newTime;

			return dt / frequency;

		}

	};

	template <typename ConcreteApplication>
	struct application :
		ConcreteApplication
	{

	public:

		static inline bool create_pipeline(D3D_FEATURE_LEVEL featureLevel, bool debug)
		{

			set_default_configuration();
			on_configuration_init(&m_configuration);

			bool success =
				create_device(featureLevel, debug) &&
				create_descriptor_heaps() &&
				create_render_context() &&
				on_render_context_created(m_renderContext) &&
				create_swap_chain(debug, get_screen_width(), get_screen_height());

			if (!success)
			{
				signal_error("Failed to create graphics pipeline.");
			}
			else
			{
				SetWindowsHook(WH_KEYBOARD, on_keyboard);
				SetWindowsHook(WH_MOUSE, on_mouse);
			}

			return success;

		}

		static inline void shutdown(void)
		{

			if (get_command_queue())
			{
				// Wait for the render to finish before shutting down
				get_command_queue().wait_for_frame(get_command_queue().get_frame_index());
			}
			
			if (m_swapChain)
			{
				on_swap_chain_released(get_device(), get_swap_chain());
			}

			if (m_device)
			{	
				on_render_context_released(m_renderContext);
			}

			base_type::shutdown();
			
		}

		static inline void main_loop(void)
		{

			MSG msg;

			do
			{

				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);

					if (!on_message(msg.hwnd, msg.message, msg.wParam, msg.lParam))
					{
						DispatchMessage(&msg);
					}

				}

				UINT64 lastFrame   = get_command_queue().get_frame_index();
				UINT64 frameToWait = lastFrame + 1 < m_configuration.swapChainBufferCount ? 0 : lastFrame + 1 - m_configuration.swapChainBufferCount;

				get_command_queue().wait_for_frame(frameToWait);
				//get_command_queue().wait_for_frame(lastFrame);
				float dt = update_fps_counter();

				on_update(dt);
				
				if (!update_swapchain())
				{
					signal_error("Failed to resize the swap chain.");
					return;
				}

				on_render(m_renderContext, get_back_buffer());

				FUSE_HR_CHECK(m_swapChain->Present1(m_configuration.syncInterval, m_configuration.presentFlags, &DXGI_PRESENT_PARAMETERS{}));

				m_renderContext.advance_frame_index();

			} while (msg.message != WM_QUIT);

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