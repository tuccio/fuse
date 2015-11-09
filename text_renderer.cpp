#include <fuse/text_renderer.hpp>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <algorithm>
#include <vector>

using namespace fuse;

bool text_renderer::init(ID3D12Device * device)
{
	return create_pso(device);
}

void text_renderer::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	ID3D12CommandAllocator * commandAllocator,
	ID3D12GraphicsCommandList * commandList,
	const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	gpu_ring_buffer * ringBuffer,
	bitmap_font * font,
	const char * text,
	const XMFLOAT2 & position,
	const XMFLOAT4 & color)
{

	struct __char
	{
		XMFLOAT2 position;
		XMFLOAT4 color;
		XMFLOAT2 texcoord;
	};

	size_t n = strlen(text);
	std::vector<__char> data;

	data.reserve(6 * n);

	XMFLOAT2 currentPosition = position;

	for (int i = 0; i < n; i++)
	{

		char c = text[i];
		auto & charInfo = (*font)[c];

		__char vertices[4];

		vertices[0].color    = color;
		vertices[0].position = currentPosition;
		vertices[0].texcoord = XMFLOAT2(charInfo.minUV[0], charInfo.minUV[1]);

		vertices[1] = vertices[0];
		vertices[1].position.x += (charInfo.rect.right - charInfo.rect.left);
		vertices[1].texcoord.x = charInfo.maxUV[0];

		vertices[2] = vertices[0];
		vertices[2].position.y += (charInfo.rect.bottom - charInfo.rect.top);
		vertices[2].texcoord.y = charInfo.maxUV[1];

		vertices[3] = vertices[2];
		vertices[3].position.x += vertices[1].position.x;
		vertices[3].texcoord.x = vertices[1].texcoord.x;

		data.push_back(vertices[0]);
		data.push_back(vertices[2]);
		data.push_back(vertices[1]);

		data.push_back(vertices[1]);
		data.push_back(vertices[2]);
		data.push_back(vertices[3]);

		currentPosition.x += charInfo.width;

	}

	D3D12_GPU_VIRTUAL_ADDRESS address;
	void * cbData = ringBuffer->allocate_constant_buffer(device, commandQueue, sizeof(__char) * data.size(), &address);
	size_t size = sizeof(__char) * data.size();

	if (cbData)
	{

		memcpy(cbData, &data[0], size);

		FUSE_HR_CHECK(commandList->Reset(commandAllocator, m_pso.get()));

		commandList->SetGraphicsRootSignature(m_rs.get());

		commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

		auto srvDescriptor = font->get_srv_descriptor();
		auto srvHeap       = font->get_srv_heap();

		commandList->SetDescriptorHeaps(1, &srvHeap);
		commandList->SetGraphicsRootDescriptorTable(1, srvDescriptor);

		commandList->OMSetRenderTargets(1, rtv, false, nullptr);

		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		D3D12_VERTEX_BUFFER_VIEW vb;

		vb.BufferLocation = address;
		vb.SizeInBytes    = size;
		vb.StrideInBytes  = sizeof(__char);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vb);

		commandList->DrawInstanced(data.size(), 1, 0, 0);

		FUSE_HR_CHECK(commandList->Close());

		commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);

	}

	// Maybe put an ortho matrix on the cbPerFrame to use

	// create a vertex buffer on the ring buffe
	// fill with the textured quads
	// render

}

bool text_renderer::create_pso(ID3D12Device * device)
{
	
	com_ptr<ID3DBlob> textVS, textPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	CD3DX12_DESCRIPTOR_RANGE srv[1];

	srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(_countof(srv), srv, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (compile_shader("shaders/text_rendering.hlsl", nullptr, "text_vs", "vs_5_0", compileFlags, &textVS) &&
		compile_shader("shaders/text_rendering.hlsl", nullptr, "text_ps", "ps_5_0", compileFlags, &textPS) &&
		reflect_input_layout(textVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		psoDesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_INV_DEST_ALPHA;
		psoDesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_DEST_ALPHA;
		psoDesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState                      = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable          = FALSE;
		psoDesc.NumRenderTargets                       = 1;
		psoDesc.RTVFormats[0]                          = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.VS                                     = { FUSE_BLOB_ARGS(textVS) };
		psoDesc.PS                                     = { FUSE_BLOB_ARGS(textPS) };
		psoDesc.InputLayout                            = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                         = m_rs.get();
		psoDesc.SampleMask                             = UINT_MAX;
		psoDesc.SampleDesc                             = { 1, 0 };
		psoDesc.PrimitiveTopologyType                  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	}

	return false;

}