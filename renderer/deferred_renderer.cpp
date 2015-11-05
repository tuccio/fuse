#include "deferred_renderer.hpp"

#include <algorithm>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

#include <d3dx12.h>

#include "cbuffer_structs.hpp"

using namespace fuse;

deferred_renderer::deferred_renderer(void) : 
	m_debug(false) { }

bool deferred_renderer::init(ID3D12Device * device, ID3D12Resource * cbPerFrame, gpu_upload_manager * uploadManager)
{

	m_hFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

	FUSE_HR_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cbFence))); // TODO: Remove

	/*m_commandList.resize(buffers);
	m_commandAllocator.resize(buffers);

	for (int i = 0; i < buffers; i++)
	{

		if (!FUSE_HR_FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]))) &&
			!FUSE_HR_FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[i].get(), nullptr, IID_PPV_ARGS(&m_commandList[i]))))
		{

			m_commandAllocator[i]->SetName(L"deferred_renderer_command_allocator");
			m_commandList[i]->SetName(L"deferred_renderer_command_list");

			m_commandList[i]->Close();

		}
		else
		{
			return false;
		}

	}
*/
	m_uploadManager = uploadManager;

	m_cbPerFrame = cbPerFrame;
	m_cbPerFrameAddress = cbPerFrame->GetGPUVirtualAddress();

	return create_psos(device);

}

void deferred_renderer::shutdown(void)
{

}

void deferred_renderer::render_init(scene * scene)
{

	/*m_bufferIndex = bufferIndex;
	m_commandAllocator[bufferIndex]->Reset();*/

	m_staticObjects.clear();

	auto staticObjectsIterators = scene->get_static_objects_iterators();

	// TODO: Culling

	for (auto it = staticObjectsIterators.first; it != staticObjectsIterators.second; it++)
	{
		m_staticObjects.push_back(&*it);
	}

	m_camera = scene->get_active_camera();

	assert(m_camera && "No active camera in the scene");

	// TODO: Maybe sort by material

}

void deferred_renderer::render_gbuffer(
	ID3D12Device * device,
	fuse::gpu_command_queue & commandQueue,
	ID3D12CommandAllocator * commandAllocator,
	ID3D12GraphicsCommandList * commandList,
	fuse::gpu_ring_buffer & ringBuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE * gbuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE * dsv)
{

	auto view              = m_camera->get_view_matrix();
	auto projection        = m_camera->get_projection_matrix();
	auto viewProjection    = XMMatrixMultiply(view, projection);
	auto invViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProjection), viewProjection);;

	/* Setup per frame constant buffer */

	cb_per_frame cbPerFrame;

	cbPerFrame.camera.position          = to_float3(m_camera->get_position());
	cbPerFrame.camera.fovy              = m_camera->get_fovy();
	cbPerFrame.camera.aspectRatio       = m_camera->get_aspect_ratio();
	cbPerFrame.camera.znear             = m_camera->get_znear();
	cbPerFrame.camera.zfar              = m_camera->get_zfar();
	cbPerFrame.camera.view              = XMMatrixTranspose(view);
	cbPerFrame.camera.projection        = XMMatrixTranspose(projection);
	cbPerFrame.camera.viewProjection    = XMMatrixTranspose(viewProjection);
	cbPerFrame.camera.invViewProjection = XMMatrixTranspose(invViewProjection);

	D3D12_SUBRESOURCE_DATA cbPerFrameData = {};

	cbPerFrameData.pData    = &cbPerFrame;
	cbPerFrameData.RowPitch = sizeof(cb_per_frame);

	m_uploadManager->upload(
		device,
		m_cbPerFrame.get(),
		0, 1,
		&cbPerFrameData,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_cbPerFrame.get(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			D3D12_RESOURCE_STATE_COPY_DEST),
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_cbPerFrame.get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	/* Clear the buffers */

	float clearColor[4] = { 0.f };

	commandList->Reset(commandAllocator, nullptr);

	commandList->ClearRenderTargetView(gbuffer[0], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[1], clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(gbuffer[2], clearColor, 0, nullptr);

	commandList->ClearDepthStencilView(*dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	commandList->Close();
	
	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);

	/* Setup the pipeline state */

	//commandList->SetPipelineState(m_gbufferPSO.get());
	
	commandList->Reset(commandAllocator, m_gbufferPSO.get());

	/* Render loop */

	material defaultMaterial;
	defaultMaterial.set_default();

	D3D12_VIEWPORT viewport = {};

	viewport.Width = m_width;
	viewport.Height = m_height;
	viewport.MaxDepth = 1.f;

	CD3DX12_RECT scissorRect(0, 0, m_width, m_height);

	UINT fenceValue = 0;

	for (auto renderable : m_staticObjects)
	{

		/*m_cbFence->SetEventOnCompletion(fenceValue, m_hFenceEvent);

		if (WaitForSingleObject(m_hFenceEvent, INFINITE) != WAIT_OBJECT_0)
		{
			FUSE_LOG_OPT_DEBUG("Failed WaitForSingleObject on fence event.");
		}*/

		/* Setup state */

		commandList->OMSetRenderTargets(3, gbuffer, false, dsv);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetGraphicsRootSignature(m_gbufferRS.get());

		commandList->SetGraphicsRootConstantBufferView(0, m_cbPerFrameAddress);
		//m_commandList->SetGraphicsRootConstantBufferView(1, m_cbPerObjectAddress);

		/* Fill buffer per object */

		auto world = renderable->get_world_matrix();;

		cb_per_object cbPerObject;

		cbPerObject.transform.world               = XMMatrixTranspose(world);
		cbPerObject.transform.worldView           = XMMatrixTranspose(XMMatrixMultiply(world, view));
		cbPerObject.transform.worldViewProjection = XMMatrixTranspose(XMMatrixMultiply(world, viewProjection));

		auto objectMaterial = renderable->get_material();

		if (objectMaterial && objectMaterial->load())
		{
			cbPerObject.material.baseColor = reinterpret_cast<const XMFLOAT3&>(objectMaterial->get_base_albedo());
			cbPerObject.material.roughness = objectMaterial->get_roughness();
			cbPerObject.material.specular  = objectMaterial->get_specular();
			cbPerObject.material.metallic  = objectMaterial->get_metallic();
		}
		else
		{
			cbPerObject.material.baseColor = reinterpret_cast<const XMFLOAT3&>(objectMaterial->get_base_albedo());
			cbPerObject.material.roughness = defaultMaterial.get_roughness();
			cbPerObject.material.specular  = defaultMaterial.get_specular();
			cbPerObject.material.metallic  = defaultMaterial.get_metallic();
		}

		//memcpy(m_cbPerObjectMap, &cbPerObject, sizeof(cb_per_object));

		void * cbData;
		com_ptr<ID3D12Resource> buffer;

		if (ringBuffer.allocate(device, commandQueue, &buffer, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(cb_per_object)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr) &&
			!FUSE_HR_FAILED(buffer->Map(0, &CD3DX12_RANGE(0, 0), &cbData)))
		{

			memcpy(cbData, &cbPerObject, sizeof(cb_per_object));
			buffer->Unmap(0, nullptr);

			commandList->SetGraphicsRootConstantBufferView(1, buffer->GetGPUVirtualAddress());

			/* Draw */

			auto mesh = renderable->get_mesh();

			if (mesh && mesh->load())
			{

				commandList->IASetVertexBuffers(0, 2, mesh->get_vertex_buffers());
				commandList->IASetIndexBuffer(&mesh->get_index_data());

				commandList->DrawIndexedInstanced(mesh->get_num_indices(), 1, 0, 0, 0);

			}			

		}

		//commandQueue->Signal(m_cbFence.get(), ++fenceValue);

	}

	commandList->Close();
	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**) &commandList);

	//commandQueue->Signal(m_cbFence.get(), 0);

}

void deferred_renderer::render_directional_light(
	ID3D12Device * device,
	ID3D12GraphicsCommandList * commandList,
	const D3D12_GPU_VIRTUAL_ADDRESS * gbuffer,
	const D3D12_CPU_DESCRIPTOR_HANDLE * rtv,
	const light * light)
{

	assert(light->type == LIGHT_TYPE_DIRECTIONAL && "Rendering a non directional light as directional");

	cb_per_light cbPerLight = {};

	cbPerLight.light.direction = light->direction;
	cbPerLight.light.luminance = light->luminance;
	cbPerLight.light.ambient   = light->ambient;

	//memcpy(m_cbPerObjectMap, &cbPerLight, sizeof(cb_per_light));

	commandList->SetPipelineState(m_finalPSO.get());
	commandList->SetGraphicsRootSignature(m_finalRS.get());

	commandList->SetGraphicsRootConstantBufferView(0, m_cbPerFrameAddress);
	//commandList->SetGraphicsRootConstantBufferView(1, m_cbPerObjectAddress);

	commandList->SetGraphicsRootShaderResourceView(1, gbuffer[0]);
	commandList->SetGraphicsRootShaderResourceView(2, gbuffer[1]);
	commandList->SetGraphicsRootShaderResourceView(3, gbuffer[2]);
	commandList->SetGraphicsRootShaderResourceView(4, gbuffer[3]);

	commandList->OMSetRenderTargets(1, rtv, false, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
	commandList->IASetVertexBuffers(0, 0, nullptr);
	commandList->DrawInstanced(4, 1, 0, 0);


}

bool deferred_renderer::create_psos(ID3D12Device * device)
{
	return create_gbuffer_pso(device) &&
		create_final_pso(device);
}

bool deferred_renderer::create_gbuffer_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> gbufferVS, gbufferPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[2];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (m_debug)
	{
		compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
	}

	if (compile_shader("shaders/deferred_shading_gbuffer.hlsl", nullptr, "gbuffer_vs", "vs_5_0", compileFlags, &gbufferVS) &&
		compile_shader("shaders/deferred_shading_gbuffer.hlsl", nullptr, "gbuffer_ps", "ps_5_0", compileFlags, &gbufferPS) &&
		reflect_input_layout(gbufferVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_gbufferRS))))
	{

		// Adjust the input desc since gpu_mesh uses slot 0 for position and
		// slot 1 for non position data (tangent space vectors, texcoords, ...)

		for (auto & elementDesc : inputLayoutVector)
		{
			if (strcmp(elementDesc.SemanticName, "POSITION"))
			{
				elementDesc.InputSlot = 1;
			}
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

		psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.NumRenderTargets         = 3;
		psoDesc.RTVFormats[0]            = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.RTVFormats[1]            = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.RTVFormats[2]            = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat                = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.VS                       = { FUSE_BLOB_ARGS(gbufferVS) };
		psoDesc.PS                       = { FUSE_BLOB_ARGS(gbufferPS) };
		psoDesc.InputLayout              = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature           = m_gbufferRS.get();
		psoDesc.SampleMask               = UINT_MAX;
		psoDesc.SampleDesc               = { 1, 0 };
		psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_gbufferPSO)));

	}

	return false;

}

bool deferred_renderer::create_final_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> quadVS, finalPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[6];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	rootParameters[2].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[5].InitAsShaderResourceView(3, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(6, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (m_debug)
	{
		compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL;
	}

	if (compile_shader("shaders/quad_vs.hlsl", nullptr, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
		compile_shader("shaders/deferred_shading_final.hlsl", nullptr, "shading_ps", "ps_5_0", compileFlags, &finalPS) &&
		reflect_input_layout(quadVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_finalRS))))
	{

		// Adjust the input desc since gpu_mesh uses slot 0 for position and
		// slot 1 for non position data (tangent space vectors, texcoords, ...)

		for (auto & elementDesc : inputLayoutVector)
		{
			if (strcmp(elementDesc.SemanticName, "POSITION"))
			{
				elementDesc.InputSlot = 1;
			}
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { };

		psoDesc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.NumRenderTargets              = 1;
		psoDesc.RTVFormats[0]                 = DXGI_FORMAT_R16G16B16A16_FLOAT;
		psoDesc.VS                            = { FUSE_BLOB_ARGS(quadVS) };
		psoDesc.PS                            = { FUSE_BLOB_ARGS(finalPS) };
		psoDesc.InputLayout                   = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                = m_finalRS.get();
		psoDesc.SampleMask                    = UINT_MAX;
		psoDesc.SampleDesc                    = { 1, 0 };
		psoDesc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_finalPSO)));

	}

	return false;

}

void deferred_renderer::on_resize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}