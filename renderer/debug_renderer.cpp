#include "debug_renderer.hpp"

using namespace fuse;

bool debug_renderer::init(ID3D12Device * device, const debug_renderer_configuration & cfg)
{
	m_configuration = cfg;
	return create_debug_pso(device) &&
		create_debug_texture_pso(device);
}

void debug_renderer::shutdown(void)
{
	m_debugPST.clear();
}

void debug_renderer::add(const aabb & boundingBox, const color_rgba & color)
{

	auto corners = boundingBox.get_corners();

	debug_line lines[] = {

		// Back face
		{ debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_LEFT]), to_float4(color)) },

		// Front face
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_LEFT]), to_float4(color)) },

		// Connect the two
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_AABB_BACK_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_AABB_FRONT_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_AABB_BACK_TOP_LEFT]), to_float4(color)) }

	};

	add_lines(lines, lines + _countof(lines));

}

void debug_renderer::add(const frustum & f, const color_rgba & color)
{

	auto corners = f.get_corners();

	debug_line lines[] = {

		// Back face
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_LEFT]), to_float4(color)) },

		// Front face
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT]), to_float4(color)) },

		// Connect the two
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT]), to_float4(color)),  debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_LEFT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_RIGHT]), to_float4(color)), debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_BOTTOM_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_RIGHT]), to_float4(color)),    debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_RIGHT]), to_float4(color)) },
		{ debug_vertex(to_float3(corners[FUSE_FRUSTUM_NEAR_TOP_LEFT]), to_float4(color)),     debug_vertex(to_float3(corners[FUSE_FRUSTUM_FAR_TOP_LEFT]), to_float4(color)) }
	};

	add_lines(lines, lines + _countof(lines));

	auto planes = f.get_planes();

	std::vector<debug_line> normals;

	std::transform(planes.begin(), planes.end(), std::back_inserter(normals), [&](const plane & p)
	{
		
		XMVECTOR plane = p.get_plane_vector();

		debug_line line;

		line.v0.position = to_float3(corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT]);
		line.v0.color    = to_float4(color);
		line.v1.position = to_float3(XMVectorAdd(corners[FUSE_FRUSTUM_NEAR_BOTTOM_LEFT], plane));
		line.v1.color    = to_float4(color);

		return line;

	});

	add_lines(normals.begin(), normals.end());
	
}

void debug_renderer::add(ID3D12Device * device, gpu_graphics_command_list & commandList, UINT bufferIndex, const render_resource & r, XMUINT2 position, float scale, bool hdr)
{

	D3D12_RESOURCE_DESC desc = r->GetDesc();
	render_resource_ptr target = render_resource_manager::get_singleton_pointer()->get_texture_2d(device, bufferIndex, desc.Format, desc.Width, desc.Height);

	commandList.resource_barrier_transition(r.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList.resource_barrier_transition(target->get(), D3D12_RESOURCE_STATE_COPY_DEST);

	commandList->CopyResource(target->get(), r.get());

	m_textures.push_back(debug_texture{ position, scale * desc.Width, scale * desc.Height, hdr, std::move(target) });

}

void debug_renderer::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{
	render_lines(device, commandQueue, commandList, ringBuffer, cbPerFrame, renderTarget, depthBuffer);
	render_textures(device, commandQueue, commandList, ringBuffer, cbPerFrame, renderTarget, depthBuffer);
}

void debug_renderer::render_lines(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{

	if (!m_lines.empty())
	{		

		D3D12_GPU_VIRTUAL_ADDRESS vbAddress;

		size_t vbSize = sizeof(debug_line) * m_lines.size();

		void * vbData = ringBuffer.allocate_constant_buffer(device, commandQueue, vbSize, &vbAddress);

		if (vbData)
		{

			auto pso = m_debugPST.get_pso_instance(device);
			memcpy(vbData, &m_lines[0], m_lines.size() * sizeof(debug_line));
			
			commandList->SetPipelineState(pso);

			commandList.resource_barrier_transition(renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			commandList.resource_barrier_transition(depthBuffer.get(), D3D12_RESOURCE_STATE_DEPTH_READ);

			commandList->OMSetRenderTargets(1, &renderTarget.get_rtv_cpu_descriptor_handle(), false, &depthBuffer.get_dsv_cpu_descriptor_handle());

			commandList->SetGraphicsRootSignature(m_debugRS.get());
			commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

			D3D12_VERTEX_BUFFER_VIEW vb;

			vb.BufferLocation = vbAddress;
			vb.SizeInBytes = vbSize;
			vb.StrideInBytes = sizeof(debug_vertex);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			commandList->IASetVertexBuffers(0, 1, &vb);

			commandList->DrawInstanced(m_lines.size() * 2, 1, 0, 0);

		}

		m_lines.clear();

	}

}

void debug_renderer::render_textures(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	const render_resource & renderTarget,
	const render_resource & depthBuffer)
{

	if (!m_textures.empty())
	{

		D3D12_GPU_VIRTUAL_ADDRESS vbAddress;

		std::vector<float> vertices;

		vertices.reserve(m_textures.size() * 16);

		for (auto & p : m_textures)
		{


			vertices.push_back(p.position.x);
			vertices.push_back(p.position.y + p.scaledHeight);

			vertices.push_back(0.f);
			vertices.push_back(1.f);

			vertices.push_back(p.position.x);
			vertices.push_back(p.position.y);

			vertices.push_back(0.f);
			vertices.push_back(0.f);

			vertices.push_back(p.position.x + p.scaledWidth);
			vertices.push_back(p.position.y + p.scaledHeight);

			vertices.push_back(1.f);
			vertices.push_back(1.f);

			vertices.push_back(p.position.x + p.scaledWidth);
			vertices.push_back(p.position.y);

			vertices.push_back(1.f);
			vertices.push_back(0.f);


		}

		void * vbData = ringBuffer.allocate_constant_buffer(device, commandQueue, vertices.size() * sizeof(float), &vbAddress);

		auto ldrPSO = m_debugTexturePST.get_pso_instance(device, { { "HDR_TEXTURE", "0" } });
		auto hdrPSO = m_debugTexturePST.get_pso_instance(device, { { "HDR_TEXTURE", "1" } });

		if (vbData)
		{

			memcpy(vbData, &vertices[0], vertices.size() * sizeof(float));

			commandList.resource_barrier_transition(renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
			commandList.resource_barrier_transition(depthBuffer.get(), D3D12_RESOURCE_STATE_DEPTH_READ);

			commandList->OMSetRenderTargets(1, &renderTarget.get_rtv_cpu_descriptor_handle(), false, &depthBuffer.get_dsv_cpu_descriptor_handle());

			commandList->SetGraphicsRootSignature(m_debugTextureRS.get());
			commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			D3D12_VERTEX_BUFFER_VIEW vb;

			vb.BufferLocation = vbAddress;
			vb.SizeInBytes    = 16 * sizeof(float);
			vb.StrideInBytes  = 4 * sizeof(float);

			for (int i = 0; i < m_textures.size(); i++)
			{

				commandList->SetPipelineState(m_textures[i].hdr ? hdrPSO : ldrPSO);

				commandList->SetGraphicsRootDescriptorTable(1, m_textures[i].texture->get_srv_gpu_descriptor_handle());
				commandList->IASetVertexBuffers(0, 1, &vb);
				commandList->DrawInstanced(4, 1, 0, 0);

				vb.BufferLocation += vb.SizeInBytes;

			}			

		}

		m_textures.clear();

	}

}

bool debug_renderer::create_debug_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[1];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

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
		debugPSODesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_SRC_ALPHA;
		debugPSODesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_INV_SRC_ALPHA;

		debugPSODesc.NumRenderTargets      = 1;
		debugPSODesc.RTVFormats[0]         = m_configuration.rtvFormat;
		debugPSODesc.DSVFormat             = m_configuration.dsvFormat;
		debugPSODesc.SampleMask            = UINT_MAX;
		debugPSODesc.SampleDesc            = { 1, 0 };
		debugPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		debugPSODesc.pRootSignature        = m_debugRS.get();

		m_debugPST = pipeline_state_template({}, debugPSODesc, "5_0");

		m_debugPST.set_vertex_shader(FUSE_LITERAL("shaders/debug.hlsl"), "debug_vs");
		m_debugPST.set_pixel_shader(FUSE_LITERAL("shaders/debug.hlsl"), "debug_ps");

		return true;

	}

	return false;

}

bool debug_renderer::create_debug_texture_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0), D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_debugTextureRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPSODesc = {};

		debugPSODesc.BlendState        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		debugPSODesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		debugPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

		debugPSODesc.DepthStencilState.DepthEnable = FALSE;

		debugPSODesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		debugPSODesc.BlendState.RenderTarget[0].SrcBlend    = D3D12_BLEND_ONE;
		debugPSODesc.BlendState.RenderTarget[0].DestBlend   = D3D12_BLEND_ZERO;

		debugPSODesc.NumRenderTargets      = 1;
		debugPSODesc.RTVFormats[0]         = m_configuration.rtvFormat;
		debugPSODesc.DSVFormat             = m_configuration.dsvFormat;
		debugPSODesc.SampleMask            = UINT_MAX;
		debugPSODesc.SampleDesc            = { 1, 0 };
		debugPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		debugPSODesc.pRootSignature        = m_debugTextureRS.get();

		m_debugTexturePST = pipeline_state_template({ { "HDR_TEXTURE", { "0", "1" } } }, debugPSODesc, "5_0");

		m_debugTexturePST.set_vertex_shader(FUSE_LITERAL("shaders/debug.hlsl"), "debug_texture_vs");
		m_debugTexturePST.set_pixel_shader(FUSE_LITERAL("shaders/debug.hlsl"), "debug_texture_ps");

		return true;

	}

	return false;

}