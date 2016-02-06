namespace fuse
{

	/* application_base */

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

		WindowingSystem::shutdown();

		if (m_debug && debugDevice)
		{
			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}

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

			return GetWindowRect(get_render_window(), &rect) &&
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

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_render_context(void)
	{
		return m_renderContext.init(m_device.get(), m_configuration.swapChainBufferCount, m_configuration.uploadHeapSize);
	}

	template <typename WindowingSystem>
	bool application_base<WindowingSystem>::create_swap_chain(bool debug, int width, int height)
	{

		m_debug = debug;

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

		swapChainDesc.BufferDesc.RefreshRate      = m_configuration.refreshRate;

		swapChainDesc.BufferDesc.Width            = width;
		swapChainDesc.BufferDesc.Height           = height;

		swapChainDesc.BufferDesc.Format           = m_configuration.swapChainFormat;

		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling          = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.BufferCount                 = m_configuration.swapChainBufferCount;
		swapChainDesc.BufferUsage                 = m_configuration.swapChainBufferUsage;
		swapChainDesc.OutputWindow                = get_render_window();
		swapChainDesc.Windowed                    = TRUE;
		swapChainDesc.SwapEffect                  = m_configuration.swapChainSwapEffect;
		swapChainDesc.Flags                       = m_configuration.swapChainFlags;

		swapChainDesc.SampleDesc                  = m_configuration.sampleDesc;

		m_swapChainBufferDesc.Format              = swapChainDesc.BufferDesc.Format;
		m_swapChainBufferDesc.Width               = width;
		m_swapChainBufferDesc.Height              = height;
		m_swapChainBufferDesc.SampleDesc          = swapChainDesc.SampleDesc;

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

	template <typename WindowingSystem>
	void application_base<WindowingSystem>::destroy_descriptor_heaps(void)
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

}