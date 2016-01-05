#include "debug_renderer.hpp"

using namespace fuse;

bool debug_renderer::init(ID3D12Device * device, const debug_renderer_configuration & cfg)
{
	m_configuration = cfg;
	return create_psos(device);
}

void debug_renderer::shutdown(void)
{
	m_debugPST.clear();
}

void debug_renderer::render_aabb(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const aabb & boudingBox,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{

	auto corners = boudingBox.get_corners();

	XMVECTOR lines[] = {

		// Back face
		corners[FUSE_AABB_BACK_BOTTOM_LEFT],  corners[FUSE_AABB_BACK_BOTTOM_RIGHT],
		corners[FUSE_AABB_BACK_BOTTOM_RIGHT], corners[FUSE_AABB_BACK_TOP_RIGHT],
		corners[FUSE_AABB_BACK_TOP_RIGHT],    corners[FUSE_AABB_BACK_TOP_LEFT],
		corners[FUSE_AABB_BACK_TOP_LEFT],     corners[FUSE_AABB_BACK_BOTTOM_LEFT],

		// Front face
		corners[FUSE_AABB_FRONT_BOTTOM_LEFT],  corners[FUSE_AABB_FRONT_BOTTOM_RIGHT],
		corners[FUSE_AABB_FRONT_BOTTOM_RIGHT], corners[FUSE_AABB_FRONT_TOP_RIGHT],
		corners[FUSE_AABB_FRONT_TOP_RIGHT],    corners[FUSE_AABB_FRONT_TOP_LEFT],
		corners[FUSE_AABB_FRONT_TOP_LEFT],     corners[FUSE_AABB_FRONT_BOTTOM_LEFT]

	};

	render_lines(device, commandQueue, commandList, ringBuffer, cbPerFrame, lines, _countof(lines) / 2, renderTarget, depthBuffer);

}

void debug_renderer::render_lines(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const XMVECTOR * vertices,
	size_t nLines,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{

	auto pso = m_debugPST.get_pso_instance(device);

	D3D12_GPU_VIRTUAL_ADDRESS vbAddress, cbAddress;

	size_t vbSize = sizeof(XMFLOAT3) * nLines * 2;

	void * vbData = ringBuffer.allocate_constant_buffer(device, commandQueue, vbSize, &vbAddress);
	void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(XMFLOAT4), &cbAddress);

	if (vbData && cbData)
	{

		commandList->SetPipelineState(pso);

		commandList.resource_barrier_transition(renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList.resource_barrier_transition(depthBuffer.get(), D3D12_RESOURCE_STATE_DEPTH_READ);

		commandList->OMSetRenderTargets(1, &renderTarget.get_rtv_cpu_descriptor_handle(), false, &depthBuffer.get_dsv_cpu_descriptor_handle());

		commandList->SetGraphicsRootSignature(m_debugRS.get());
		commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);
		commandList->SetGraphicsRootConstantBufferView(1, cbAddress);

		D3D12_VERTEX_BUFFER_VIEW vb;

		vb.BufferLocation = vbAddress;
		vb.SizeInBytes    = vbSize;
		vb.StrideInBytes  = sizeof(XMFLOAT3);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &vb);

		commandList->DrawInstanced(nLines * 2, 1, 0, 0);

	}

}

bool debug_renderer::create_psos(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_debugRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPSODesc = {};

		debugPSODesc.BlendState        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		debugPSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		debugPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		// Lookup depth buffer
		debugPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		debugPSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		debugPSODesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
		debugPSODesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_ONE;

		debugPSODesc.NumRenderTargets      = 1;
		debugPSODesc.RTVFormats[0]         = m_configuration.rtvFormat;
		debugPSODesc.DSVFormat             = m_configuration.dsvFormat;
		debugPSODesc.SampleMask            = UINT_MAX;
		debugPSODesc.SampleDesc            = { 1, 0 };
		debugPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		debugPSODesc.pRootSignature        = m_debugRS.get();

		m_debugPST = pipeline_state_template({}, debugPSODesc, "5_0");

		m_debugPST.set_vertex_shader("shaders/debug.hlsl", "debug_vs");
		m_debugPST.set_pixel_shader("shaders/debug.hlsl", "debug_ps");

		return true;

	}

	return false;

}