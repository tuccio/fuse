#define FUSE_WINDOW_MIN_SIZE_X 320
#define FUSE_WINDOW_MIN_SIZE_Y 240

namespace fuse
{

	/* Window class and callbacks */

	template <typename WindowingSystem>
	LRESULT CALLBACK application_base<WindowingSystem>::window_proc(HWND hWnd,
		UINT   uMsg,
		WPARAM wParam,
		LPARAM lParam)
	{

		switch (uMsg)
		{

		case WM_DESTROY:

			PostQuitMessage(0);
			break;

		case WM_SIZE:

		{

			if (wParam == SIZE_MINIMIZED)
			{
				// Maybe sleep
			}
			else
			{

				RECT clientRect;
				GetClientRect(hWnd, &clientRect);

				m_screenWidth = clientRect.right - clientRect.left;
				m_screenHeight = clientRect.bottom - clientRect.top;

				m_resizeSwapChain = true;

			}


			break;

		}

		break;


		case WM_GETMINMAXINFO:

		{

			MINMAXINFO * minMaxInfo = reinterpret_cast<MINMAXINFO *>(lParam);

			minMaxInfo->ptMinTrackSize.x = FUSE_WINDOW_MIN_SIZE_X;
			minMaxInfo->ptMinTrackSize.y = FUSE_WINDOW_MIN_SIZE_Y;

			return 0;

		}

		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	}

	/* application_base */

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::init(HINSTANCE hInstance, bool silent)
	{

		m_silent = silent;

		if (!m_initialized)
		{

			m_hInstance = hInstance;

			WNDCLASSEX wndClass = { 0 };

			wndClass.cbSize = sizeof(WNDCLASSEX);
			wndClass.style = CS_HREDRAW | CS_VREDRAW;
			wndClass.lpfnWndProc = window_proc;
			wndClass.hInstance = hInstance;
			wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
			wndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
			wndClass.lpszClassName = FUSE_WINDOW_CLASS;

			if (!(m_initialized = RegisterClassEx(&wndClass)))
			{
				FUSE_LOG_OPT_DEBUG("Failed to register window class.");
			}

		}

		return m_initialized;

	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::shutdown(void)
	{

		com_ptr<ID3D12DebugDevice> debugDevice;

		if (m_swapChain)
		{
			set_fullscreen(false);
			m_swapChain.reset();
		}

		if (m_device)
		{
			m_device->QueryInterface(IID_PPV_ARGS(&debugDevice));
			m_device.reset();
		}

		m_renderContext.shutdown();

		release_swap_chain_buffers();
		destroy_descriptor_heaps();

		UnregisterClass(FUSE_WINDOW_CLASS, m_hInstance);
		m_initialized = false;

		if (m_debug && debugDevice)
		{
			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}

	}

	template <typename WindowingSystem>
	LRESULT CALLBACK application_base<WindowingSystem>::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
	{
		return CallNextHookEx(NULL, code, wParam, lParam);
	}

	template <typename WindowingSystem>
	LRESULT CALLBACK application_base<WindowingSystem>::on_mouse(int code, WPARAM wParam, LPARAM lParam)
	{
		return CallNextHookEx(NULL, code, wParam, lParam);
	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_window(int width, int height, const char * caption)
	{

		assert(m_hInstance && m_initialized && "Cannot create window before initializing.");
		assert(!m_hWnd && "Window already exists.");

		RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		m_hWnd = CreateWindowEx(0,
			FUSE_WINDOW_CLASS,
			caption,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			m_hInstance,
			nullptr);

		if (!m_hWnd)
		{
			FUSE_HR_LOG(GetLastError());
			return false;
		}
		else
		{
			ShowWindow(m_hWnd, SW_NORMAL);
			return true;
		}

	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::destroy_window(void)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}

	template <typename WindowingSystem>
	float application_base<WindowingSystem>::get_fps(void)
	{

		float time = 0.f;

		for (int i = 0; i < FUSE_FPS_SAMPLES; i++)
		{
			time += m_frameSamples[i];
		}

		return time / FUSE_FPS_SAMPLES;

	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::is_fullscreen(void)
	{
		BOOL fullscreen;
		FUSE_HR_CHECK(m_swapChain->GetFullscreenState(&fullscreen, nullptr));
		return fullscreen;
	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::set_fullscreen(bool fullscreen)
	{
		FUSE_HR_CHECK(m_swapChain->SetFullscreenState(fullscreen, nullptr));
	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::set_cursor(bool clippedFullScreen, bool hidden)
	{

		if (clippedFullScreen)
		{

			RECT rect;

			return GetWindowRect(m_hWnd, &rect) &&
				ClipCursor(&rect) &&
				ShowCursor(!hidden);

		}
		else
		{
			return ClipCursor(nullptr) &&
				ShowCursor(!hidden);
		}

	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::set_default_configuration(void)
	{

		m_configuration = application_config{};

		m_configuration.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_configuration.swapChainBufferCount = 2;
		m_configuration.swapChainBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_configuration.sampleDesc.Count = 1;
		m_configuration.refreshRate.Denominator = 1;
		m_configuration.swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		m_configuration.swapChainSwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		m_configuration.maxDSV = 64;
		m_configuration.maxCBVUAVSRV = 2048;
		m_configuration.maxRTV = 2048;

		m_configuration.uploadHeapSize = 1 << 20;

	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_device(D3D_FEATURE_LEVEL featureLevel, bool debug)
	{

		if (debug)
		{

			com_ptr<ID3D12Debug> debugInterface;

			if (FUSE_HR_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
			{
				return false;
			}

			debugInterface->EnableDebugLayer();

		}

		if (!FUSE_HR_FAILED(D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&m_device))))
		{
			m_device->SetName(L"fuse_device");
			return true;
		}

		return false;

	}
	bool application_base::create_render_context(void)
	{
		return m_renderContext.init(m_device.get(), m_configuration.swapChainBufferCount, m_configuration.uploadHeapSize);
	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_swap_chain(bool debug, int width, int height)
	{

		m_debug = debug;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

		swapChainDesc.BufferDesc.RefreshRate = m_configuration.refreshRate;

		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;

		swapChainDesc.BufferDesc.Format = m_configuration.swapChainFormat;

		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.BufferCount = m_configuration.swapChainBufferCount;
		swapChainDesc.BufferUsage = m_configuration.swapChainBufferUsage;
		swapChainDesc.OutputWindow = m_hWnd;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = m_configuration.swapChainSwapEffect;
		swapChainDesc.Flags = m_configuration.swapChainFlags;

		swapChainDesc.SampleDesc = m_configuration.sampleDesc;

		m_swapChainBufferDesc.Format = swapChainDesc.BufferDesc.Format;
		m_swapChainBufferDesc.Width = width;
		m_swapChainBufferDesc.Height = height;
		m_swapChainBufferDesc.SampleDesc = swapChainDesc.SampleDesc;

		com_ptr<IDXGIFactory2> dxgiFactory;

		UINT dxgiFlags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;

		com_ptr<IDXGISwapChain> swapChain;

		bool success = !FUSE_HR_FAILED(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&dxgiFactory))) &&
			!FUSE_HR_FAILED(dxgiFactory->CreateSwapChain(get_command_queue().get(), &swapChainDesc, &swapChain));

		if (success)
		{
			swapChain.as(m_swapChain);
			m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();
			success = get_swap_chain_buffers();
		}

		return success;

	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::resize_swap_chain(int width, int height)
	{

		// Wait for all the frames to be completed before resizing
		get_command_queue().wait_for_frame(get_command_queue().get_frame_index());

		m_swapChainBufferDesc.Width = width;
		m_swapChainBufferDesc.Height = height;

		release_swap_chain_buffers();

		return !FUSE_HR_FAILED(m_swapChain->ResizeBuffers(m_configuration.swapChainBufferCount, width, height, m_configuration.swapChainFormat, 0)) &&
			get_swap_chain_buffers();

	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_descriptor_heaps(void)
	{

		return m_depthStencilDescriptorHeap.init(m_device.get(), m_configuration.maxDSV) &&
			m_renderTargetDescriptorHeap.init(m_device.get(), m_configuration.maxRTV) &&
			m_shaderDescriptorHeap.init(m_device.get(), m_configuration.maxCBVUAVSRV);

	}

	void application_base::destroy_descriptor_heaps(void)
	{
		m_depthStencilDescriptorHeap.shutdown();
		m_renderTargetDescriptorHeap.shutdown();
		m_shaderDescriptorHeap.shutdown();
	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::get_swap_chain_buffers(void)
	{

		m_renderTargets.resize(m_configuration.swapChainBufferCount);

		for (int i = 0; i < m_configuration.swapChainBufferCount; i++)
		{

			if (!FUSE_HR_FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
			{
				FUSE_HR_CHECK(m_renderTargets[i]->SetName(L"fuse_swapchain_buffer"));
				m_globalState.set_state(m_renderTargets[i].get(), D3D12_RESOURCE_STATE_PRESENT);
				m_renderTargets[i].create_render_target_view(m_device.get());
			}
			else
			{
				return false;
			}

		}

		return true;

	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::release_swap_chain_buffers(void)
	{
		m_renderTargets.clear();
	}

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::signal_error(const char * error)
	{
		if (!m_silent) MessageBox(m_hWnd, error, "Fuse error", MB_ICONERROR);
	}

}

#include <fuse/application.inl>