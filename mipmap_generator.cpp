#include <fuse/mipmap_generator.hpp>
#include <fuse/pipeline_state.hpp>

using namespace fuse;

mipmap_generator::mipmap_generator(ID3D12Device * device)
{

	if (!init(device))
	{
		FUSE_LOG_OPT("mipmap_generator", "Failed to construct.");
	}
	
}

bool mipmap_generator::generate_mipmaps(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	ID3D12Resource * resource)
{

	D3D12_RESOURCE_DESC desc = resource->GetDesc();

	// Get the PSO

	ID3D12PipelineState * pso;

	auto it = m_psos.find(desc.Format);

	if (it == m_psos.end())
	{

		if (!create_pso(device, desc.Format))
		{
			return false;
		}

		pso = m_psos[desc.Format].get();

	}
	else
	{
		pso = it->second.get();
	}

	// Create the mip 0 SRV
	// For each mip, create the RTV and render

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};

	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.NumDescriptors = desc.MipLevels - 1;
	rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	com_ptr<ID3D12DescriptorHeap> srvHeap;
	com_ptr<ID3D12DescriptorHeap> rtvHeap;

	FUSE_HR_CHECK(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));
	FUSE_HR_CHECK(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format                    = desc.Format;
	srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels       = 1;

	device->CreateShaderResourceView(resource, &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapCPUDesc = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHeapGPUDesc = srvHeap->GetGPUDescriptorHandleForHeapStart();

	UINT rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UINT srvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	commandList->SetPipelineState(pso);
	commandList->SetGraphicsRootSignature(m_rs.get());

	commandList.resource_barrier_transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Need an SRV access to the mip 0
	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0));

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};

	viewport.MaxDepth = 1.f;

	UINT width = desc.Width;
	UINT height = desc.Height;

	commandList->SetDescriptorHeaps(1, &srvHeap);

	const float black[4] = { 0 };

	for (int i = 0; i < desc.MipLevels - 1; i++)
	{

		width /= 2;
		height /= 2;

		viewport.Width     = width;
		viewport.Height    = height;
		scissorRect.right  = width;
		scissorRect.bottom = height;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};

		rtvDesc.Format               = desc.Format;
		rtvDesc.Texture2D.MipSlice   = 1 + i;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvHeapCPUDesc, i, rtvDescSize);

		device->CreateRenderTargetView(resource, &rtvDesc, rtv);

		commandList->ClearRenderTargetView(rtv, black, 0, nullptr);

		commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		commandList->SetGraphicsRootDescriptorTable(0, srvHeapGPUDesc);

		commandList->DrawInstanced(4, 1, 0, 0);

	}

	// Transition to make the state coherent to the global state for each subresource

	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, 0));

	commandQueue.safe_release(srvHeap.get());
	commandQueue.safe_release(rtvHeap.get());

	return true;

}

bool mipmap_generator::init(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[1];

	CD3DX12_DESCRIPTOR_RANGE srvDescRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	rootParameters[0].InitAsDescriptorTable(1, &srvDescRange);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	m_mipID = 0;
	m_hFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

	return m_hFenceEvent &&
	       !FUSE_HR_FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))) &&
	       compile_shader("shaders/quad_vs.hlsl", nullptr, "quad_vs", "vs_5_0", compileFlags, &m_vs) &&
	       compile_shader("shaders/mipmap_generator.hlsl", nullptr, "mipmap_ps", "ps_5_0", compileFlags, &m_ps) &&
	       reflect_input_layout(m_vs.get(), std::back_inserter(m_inputLayoutVector), &m_vsReflection) &&
	       !FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
	       !FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs)));

}

bool mipmap_generator::create_pso(ID3D12Device * device, DXGI_FORMAT format)
{

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_ONE;
	psoDesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState                      = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable          = FALSE;
	psoDesc.NumRenderTargets                       = 1;
	psoDesc.RTVFormats[0]                          = format;
	psoDesc.VS                                     = { FUSE_BLOB_ARGS(m_vs) };
	psoDesc.PS                                     = { FUSE_BLOB_ARGS(m_ps) };
	psoDesc.InputLayout                            = make_input_layout_desc(m_inputLayoutVector);
	psoDesc.pRootSignature                         = m_rs.get();
	psoDesc.SampleMask                             = UINT_MAX;
	psoDesc.SampleDesc                             = { 1, 0 };
	psoDesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_psos[format])));

}