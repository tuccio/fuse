#include <fuse/rocket_interface.hpp>
#include <fuse/gpu_global_resource_state.hpp>
#include <fuse/resource_factory.hpp>

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>
#include <fuse/core.hpp>

#include <fuse/descriptor_heap.hpp>

#include <algorithm>
#include <vector>

#include <chrono>

using namespace fuse;

static auto g_startTime = std::chrono::steady_clock::now();

#define FUSE_USE_COLOR   (1)
#define FUSE_USE_TEXTURE (2)

rocket_interface::rocket_interface(void) :
	m_commandQueue(nullptr),
	m_ringBuffer(nullptr),
	m_width(256),
	m_height(256) { }

rocket_interface::~rocket_interface(void)
{
	shutdown();
}

bool rocket_interface::init(ID3D12Device * device, const rocket_interface_configuration & cfg)
{

	m_device        = device;

	m_lastGeometryHandle = 0;
	m_lastTextureHandle = 0;

	m_scissorEnabled = false;

	m_configuration = cfg;

	return create_pso();

}

void rocket_interface::shutdown(void)
{

	if (m_commandQueue)
	{

		m_commandQueue  = nullptr;
		m_ringBuffer    = nullptr;

		m_textures.clear();
		m_geometry.clear();

	}

}

void rocket_interface::on_resize(UINT width, UINT height)
{
	m_width  = width;
	m_height = height;
}

Rocket::Core::CompiledGeometryHandle rocket_interface::CompileGeometry(Rocket::Core::Vertex * vertices, int num_vertices, int * indices, int num_indices, Rocket::Core::TextureHandle texture)
{

	Rocket::Core::CompiledGeometryHandle handle = 0u;

	std::vector<float> vertexData;

	vertexData.reserve(num_vertices * 8 + num_indices);

	auto texIt = m_textures.find(texture);

	std::for_each(vertices, vertices + num_vertices, [&](const Rocket::Core::Vertex & v)
	{

		const float normalization = 1.f / 255.f;

		vertexData.push_back(v.position.x);
		vertexData.push_back(v.position.y);

		vertexData.push_back(v.colour.red * normalization);
		vertexData.push_back(v.colour.green * normalization);
		vertexData.push_back(v.colour.blue * normalization);
		vertexData.push_back(v.colour.alpha * normalization);

		vertexData.push_back(v.tex_coord.x);
		vertexData.push_back(v.tex_coord.y);

	});

	std::for_each(indices, indices + num_indices, [&](const int & index)
	{
		vertexData.push_back(reinterpret_cast<const float &>(index));
	});

	com_ptr<ID3D12Resource> buffer;

	size_t bufferSize = sizeof(float) * vertexData.size();

	if (gpu_global_resource_state::get_singleton_pointer()->create_committed_resource(
			m_device,
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(float) * vertexData.size()),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&buffer)))
	{

		UINT64 heapOffset;
		void * data = m_ringBuffer->allocate_constant_buffer(m_device, *m_commandQueue, bufferSize, nullptr, &heapOffset);

		memcpy(data, &vertexData[0], bufferSize);

		gpu_upload_buffer(*m_commandQueue, *m_commandList, buffer.get(), 0, m_ringBuffer->get_heap(), heapOffset, bufferSize);

		D3D12_GPU_VIRTUAL_ADDRESS dataAddress = buffer->GetGPUVirtualAddress();

		compiled_geometry geometry;

		geometry.buffer = std::move(buffer);

		geometry.vertexBufferView.BufferLocation = dataAddress;
		geometry.vertexBufferView.StrideInBytes  = sizeof(float) * 8;
		geometry.vertexBufferView.SizeInBytes    = geometry.vertexBufferView.StrideInBytes * num_vertices;

		geometry.indexBufferView.BufferLocation = dataAddress + geometry.vertexBufferView.SizeInBytes;
		geometry.indexBufferView.SizeInBytes    = sizeof(uint32_t) * num_indices;
		geometry.indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

		geometry.numIndices = num_indices;

		geometry.texture = texture;

		handle = ++m_lastGeometryHandle;

		m_geometry.emplace(handle, geometry);

	}

	return handle;

}

void rocket_interface::render_begin(
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS cbPerFrame,
	ID3D12Resource * renderTarget,
	const D3D12_CPU_DESCRIPTOR_HANDLE & rtv)
{

	m_commandQueue = std::addressof(commandQueue);
	m_commandList  = std::addressof(commandList);
	m_ringBuffer   = std::addressof(ringBuffer);

	commandList->SetPipelineState(m_pso.get());
	commandList->SetGraphicsRootSignature(m_rs.get());

	commandList.resource_barrier_transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->SetGraphicsRootConstantBufferView(0, cbPerFrame);

	commandList->OMSetRenderTargets(1, &rtv, true, nullptr);

	commandList->RSSetViewports(1, &make_fullscreen_viewport(m_width, m_height));
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

void rocket_interface::render_end(void)
{
}

void rocket_interface::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f & translation)
{

	auto geomIt = m_geometry.find(geometry);

	if (geomIt != m_geometry.end())
	{
		
		gpu_graphics_command_list & commandList = *m_commandList;

		struct { float translation[2]; uint32_t colorFlags; } objectBufferData;
		
		objectBufferData.translation[0] = translation.x;
		objectBufferData.translation[1] = translation.y;

		auto texIt = m_textures.find(geomIt->second.texture);

		if (texIt != m_textures.end())
		{

			objectBufferData.colorFlags = FUSE_USE_TEXTURE;

			if (texIt->second.generated)
			{
				objectBufferData.colorFlags |= FUSE_USE_COLOR;
			}

			ID3D12Resource * r = texIt->second.texture->get_resource();

			commandList.resource_barrier_transition(r, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->SetGraphicsRootDescriptorTable(2, cbv_uav_srv_descriptor_heap::get_singleton_pointer()->get_gpu_descriptor_handle(texIt->second.srvToken));
		}
		else
		{
			objectBufferData.colorFlags = FUSE_USE_COLOR;
		}

		D3D12_GPU_VIRTUAL_ADDRESS cbPerObject;
		void * cbData = m_ringBuffer->allocate_constant_buffer(m_device, *m_commandQueue, sizeof(objectBufferData), &cbPerObject);

		memcpy(cbData, &objectBufferData, sizeof(objectBufferData));

		commandList->SetGraphicsRootConstantBufferView(1, cbPerObject);

		commandList->IASetVertexBuffers(0, 1, &geomIt->second.vertexBufferView);
		commandList->IASetIndexBuffer(&geomIt->second.indexBufferView);

		if (m_scissorEnabled)
		{
			commandList->RSSetScissorRects(1, &m_scissorRect);
		}
		else
		{
			commandList->RSSetScissorRects(1, &make_fullscreen_scissor_rect(m_width, m_height));
		}

		commandList->DrawIndexedInstanced(geomIt->second.numIndices, 1, 0, 0, 0);

	}

}

void rocket_interface::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry)
{
	
	auto it = m_geometry.find(geometry);

	if (it != m_geometry.end())
	{
		m_commandQueue->safe_release(it->second.buffer.get());
		m_geometry.erase(it);
	}

}

void rocket_interface::EnableScissorRegion(bool enable)
{
	m_scissorEnabled = enable;
}

void rocket_interface::SetScissorRegion(int x, int y, int width, int height)
{
	m_scissorRect = CD3DX12_RECT(x, y, x + width, y + height);
}

bool rocket_interface::LoadTexture(Rocket::Core::TextureHandle & texture_handle, Rocket::Core::Vector2i & texture_dimensions, const Rocket::Core::String & source)
{

	string_t sourceStr = to_string_t(source.CString());
	texture_ptr tex = resource_factory::get_singleton_pointer()->create<texture>(FUSE_RESOURCE_TYPE_TEXTURE, sourceStr.c_str());

	texture_handle = register_texture(tex);

	texture_dimensions.x = tex->get_width();
	texture_dimensions.y = tex->get_height();

	return texture_handle != 0;

}

bool rocket_interface::GenerateTexture(Rocket::Core::TextureHandle & texture_handle, const Rocket::Core::byte * source, const Rocket::Core::Vector2i & source_dimensions)
{

	// Needs the resource not to be unloaded until it's not used anymore

	static dynamic_resource_loader fakeImageLoader(
		[](resource * r) { return true; },
		[](resource * r) { static_cast<image*>(r)->clear(); });

	static dynamic_resource_loader fakeTextureLoader(
		[](resource * r) { return true; },
		[this](resource * r) { static_cast<texture*>(r)->clear(*m_commandQueue); });

	image_ptr   img = resource_factory::get_singleton_pointer()->create<image>(FUSE_RESOURCE_TYPE_IMAGE, FUSE_LITERAL(""), default_parameters(), &fakeImageLoader);
	texture_ptr tex = resource_factory::get_singleton_pointer()->create<texture>(FUSE_RESOURCE_TYPE_TEXTURE, FUSE_LITERAL(""), default_parameters(), &fakeTextureLoader);

	if (img->create(FUSE_IMAGE_FORMAT_R8G8B8A8_UINT, source_dimensions.x, source_dimensions.y, source) &&
		tex->create(m_device, *m_commandQueue, *m_commandList, *m_ringBuffer, img.get()))
	{
		texture_handle = register_texture(tex);
		m_textures[texture_handle].generated = true;
		return texture_handle != 0;
	}

	return false;

}

Rocket::Core::TextureHandle rocket_interface::register_texture(const texture_ptr & texture)
{

	Rocket::Core::TextureHandle texture_handle = 0;

	if (texture && texture->load())
	{

		static auto srvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		texture_handle = ++m_lastTextureHandle;

		loaded_texture loadedTexture;

		loadedTexture.texture   = texture;
		loadedTexture.srvToken  = cbv_uav_srv_descriptor_heap::get_singleton_pointer()->create_shader_resource_view(m_device, texture->get_resource());
		loadedTexture.generated = false;

		m_textures.emplace(texture_handle, loadedTexture);

	}

	return texture_handle;

}

void rocket_interface::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
	
	auto it = m_textures.find(texture_handle);

	if (it != m_textures.end())
	{
		cbv_uav_srv_descriptor_heap::get_singleton_pointer()->free(it->second.srvToken);
		m_commandQueue->safe_release(it->second.texture->get_resource());
		m_textures.erase(it);
	}

}

bool rocket_interface::create_pso(void)
{

	com_ptr<ID3DBlob> rocketVS, rocketPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[3];

	CD3DX12_DESCRIPTOR_RANGE texSRV;

	texSRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsDescriptorTable(1, &texSRV, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[] = {
		CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT)
	};

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	if (compile_shader(FUSE_LITERAL("shaders/rocket.hlsl"), nullptr, "rocket_vs", "vs_5_0", compileFlags, &rocketVS) &&
		compile_shader(FUSE_LITERAL("shaders/rocket.hlsl"), nullptr, "rocket_ps", "ps_5_0", compileFlags, &rocketPS) &&
		reflect_input_layout(rocketVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(m_device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState                    = m_configuration.blendDesc;
		psoDesc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode      = D3D12_CULL_MODE_NONE;
		psoDesc.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.NumRenderTargets              = 1;
		psoDesc.RTVFormats[0]                 = m_configuration.rtvFormat;
		psoDesc.VS                            = { FUSE_BLOB_ARGS(rocketVS) };
		psoDesc.PS                            = { FUSE_BLOB_ARGS(rocketPS) };
		psoDesc.InputLayout                   = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                = m_rs.get();
		psoDesc.SampleMask                    = UINT_MAX;
		psoDesc.SampleDesc                    = { 1, 0 };
		psoDesc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	}

	return false;

}

float rocket_interface::GetElapsedTime(void)
{
	auto currentTime = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::seconds>(currentTime - g_startTime).count();
}

int rocket_interface::TranslateString(Rocket::Core::String & translated, const Rocket::Core::String & input)
{
	translated = input;
	return 0;
}

bool rocket_interface::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String & message)
{
	string_t messageStr = to_string_t(message.CString());
	FUSE_LOG_OPT(FUSE_LITERAL("libRocket"), messageStr.c_str());
	return true;
}