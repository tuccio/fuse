#include <fuse/application.hpp>

#define FUSE_WINDOW_MIN_SIZE_X 320
#define FUSE_WINDOW_MIN_SIZE_Y 240

using namespace fuse;

/* Static variables definition */

bool                                 application_base::m_initialized;
bool                                 application_base::m_silent;
bool                                 application_base::m_debug;
							         
HINSTANCE                            application_base::m_hInstance;
HWND                                 application_base::m_hWnd;
							         
int                                  application_base::m_screenWidth;
int                                  application_base::m_screenHeight;
							         
bool                                 application_base::m_resizeSwapChain;
							         
com_ptr<ID3D12Device>                application_base::m_device;
com_ptr<IDXGISwapChain3>             application_base::m_swapChain;

gpu_command_queue                    application_base::m_commandQueue;
							         
UINT                                 application_base::m_bufferIndex;
							         
DXGI_SURFACE_DESC                    application_base::m_swapChainBufferDesc;
application_config                   application_base::m_configuration;

com_ptr<ID3D12DescriptorHeap>        application_base::m_rtvHeap;
UINT                                 application_base::m_rtvDescriptorSize;
UINT                                 application_base::m_dsvDescriptorSize;
UINT                                 application_base::m_srvDescriptorSize;

std::vector<com_ptr<ID3D12Resource>> application_base::m_renderTargets;

float                                application_base::m_frameSamples[FUSE_FPS_SAMPLES];

/* Window class and callbacks */

LRESULT CALLBACK application_base::window_proc(HWND   hWnd,
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

bool application_base::init(HINSTANCE hInstance, bool silent)
{

	m_silent = silent;

	if (!m_initialized)
	{

		m_hInstance = hInstance;

		WNDCLASSEX wndClass = { 0 };

		wndClass.cbSize        = sizeof(WNDCLASSEX);
		wndClass.style         = CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc   = window_proc;
		wndClass.hInstance     = hInstance;
		wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wndClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
		wndClass.lpszClassName = FUSE_WINDOW_CLASS;

		if  (!(m_initialized = RegisterClassEx(&wndClass)))
		{
			FUSE_LOG_OPT_DEBUG("Failed to register window class.");
		}

	}

	return m_initialized;

}

void application_base::shutdown(void)
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

	m_commandQueue.shutdown();

	UnregisterClass(FUSE_WINDOW_CLASS, m_hInstance);
	m_initialized = false;

	if (m_debug && debugDevice)
	{
		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	}

}

LRESULT CALLBACK application_base::on_keyboard(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT CALLBACK application_base::on_mouse(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}

bool application_base::create_window(int width, int height, const char * caption)
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

void application_base::destroy_window(void)
{
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}

float application_base::get_fps(void)
{
	
	float time = 0.f;

	for (int i = 0; i < FUSE_FPS_SAMPLES; i++)
	{
		time += m_frameSamples[i];
	}

	return time / FUSE_FPS_SAMPLES;

}

bool application_base::is_fullscreen(void)
{
	BOOL fullscreen;
	FUSE_HR_CHECK(m_swapChain->GetFullscreenState(&fullscreen, nullptr));
	return fullscreen;
}

void application_base::set_fullscreen(bool fullscreen)
{
	FUSE_HR_CHECK(m_swapChain->SetFullscreenState(fullscreen, nullptr));
}

bool application_base::set_cursor(bool clippedFullScreen, bool hidden)
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

void application_base::set_default_configuration(void)
{

	m_configuration = application_config { };

	m_configuration.swapChainFormat         = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_configuration.swapChainBufferCount    = 2;
	m_configuration.swapChainBufferUsage    = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_configuration.sampleDesc.Count        = 1;
	m_configuration.refreshRate.Denominator = 1;

}

bool application_base::create_device(D3D_FEATURE_LEVEL featureLevel, bool debug)
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

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		return true;

	}

	return false;

}

bool application_base::create_command_queue(void)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = { };
	return m_commandQueue.init(m_device.get());
}

bool application_base::create_swap_chain(bool debug, int width, int height)
{

	m_debug = debug;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { };

	swapChainDesc.BufferDesc.RefreshRate      = m_configuration.refreshRate;

	swapChainDesc.BufferDesc.Width            = width;
	swapChainDesc.BufferDesc.Height           = height;

	swapChainDesc.BufferDesc.Format           = m_configuration.swapChainFormat;

	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.BufferCount                 = m_configuration.swapChainBufferCount;
	swapChainDesc.BufferUsage                 = m_configuration.swapChainBufferUsage;
	swapChainDesc.OutputWindow                = m_hWnd;
	swapChainDesc.Windowed                    = TRUE;
	swapChainDesc.SwapEffect                  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.Flags                       = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	swapChainDesc.SampleDesc                  = m_configuration.sampleDesc;

	m_swapChainBufferDesc.Format     = swapChainDesc.BufferDesc.Format;
	m_swapChainBufferDesc.Width      = width;
	m_swapChainBufferDesc.Height     = height;
	m_swapChainBufferDesc.SampleDesc = swapChainDesc.SampleDesc;

	com_ptr<IDXGIFactory2> dxgiFactory;

	UINT dxgiFlags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;

	com_ptr<IDXGISwapChain> swapChain;

	bool success = !FUSE_HR_FAILED(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&dxgiFactory))) &&
	               !FUSE_HR_FAILED(dxgiFactory->CreateSwapChain(m_commandQueue.get(), &swapChainDesc, &swapChain));

	if (success)
	{
		swapChain.as(m_swapChain);
		m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();
		success = get_swap_chain_buffers() && create_render_target_views();
	}

	return success;

}

bool application_base::resize_swap_chain(int width, int height)
{

	// Wait for all the frames to be completed before resizing
	m_commandQueue.wait_for_frame(m_commandQueue.get_frame_index());

	m_swapChainBufferDesc.Width  = width;
	m_swapChainBufferDesc.Height = height;

	release_swap_chain_buffers();

	return !FUSE_HR_FAILED(m_swapChain->ResizeBuffers(m_configuration.swapChainBufferCount, width, height, m_configuration.swapChainFormat, 0)) &&
	       get_swap_chain_buffers() &&
	       update_render_target_views();

}

bool application_base::create_render_target_views(void)
{

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.NumDescriptors = m_configuration.swapChainBufferCount;
	rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	return !FUSE_HR_FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap))) &&
	       update_render_target_views();
}

bool application_base::update_render_target_views(void)
{

	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (int i = 0; i < m_configuration.swapChainBufferCount; i++)
	{
		m_device->CreateRenderTargetView(m_renderTargets[i].get(), nullptr, rtvDescriptor);
		rtvDescriptor.ptr += m_rtvDescriptorSize;
	}

	return true;

}

void application_base::release_render_target_views(void)
{
	m_rtvHeap.reset();
}

bool application_base::get_swap_chain_buffers(void)
{

	m_renderTargets.resize(m_configuration.swapChainBufferCount);

	for (int i = 0; i < m_configuration.swapChainBufferCount; i++)
	{

		if (!FUSE_HR_FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]))))
		{
			FUSE_HR_CHECK(m_renderTargets[i]->SetName(L"fuse_swapchain_buffer"));
		}
		else
		{
			return false;
		}

	}

	return true;

}

void application_base::release_swap_chain_buffers(void)
{
	m_renderTargets.clear();
}

void application_base::signal_error(const char * error)
{
	if (!m_silent) MessageBox(m_hWnd, error, "Fuse error", MB_ICONERROR);
}