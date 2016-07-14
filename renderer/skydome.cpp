#include "skydome.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

using namespace fuse;

#define SKYDOME_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT

bool skydome::init(ID3D12Device * device, uint32_t width, uint32_t height, uint32_t buffers)
{

	m_uptodate = false;

	/* uv */

	m_skydomes.resize(buffers);

	m_skydomeResolution = uint2(2048, 768);

	for (auto & skydome : m_skydomes)
	{

		if (skydome.create(
			device,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				SKYDOME_FORMAT,
				m_skydomeResolution.x, m_skydomeResolution.y,
				1, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&CD3DX12_CLEAR_VALUE(SKYDOME_FORMAT, color_rgba::zero)))
		{

			skydome.create_render_target_view(device);
			skydome.create_shader_resource_view(device, nullptr);

			skydome->SetName(L"fuse_skydome_texture");

		}
		else
		{
			shutdown();
			return false;
		}

	}

	return true;

}

void skydome::shutdown(void)
{
	m_skydomes.clear();
}

render_resource & skydome::get_current_skydome(void)
{
	return m_skydomes[m_lastUpdatedBuffer];
}

static float3 compute_sun_direction(float elevation, float azimuth)
{

	float sinAzimuth, cosAzimuth;
	float sinElevation, cosElevation;

	sinAzimuth = std::sin(azimuth);
	cosAzimuth = std::cos(azimuth);

	sinElevation = std::sin(elevation);
	cosElevation = std::cos(elevation);

	float3 direction;

	direction.y = sinElevation;
	direction.x = cosElevation * cosAzimuth;
	direction.z = cosElevation * sinAzimuth;

	return direction;
}

light skydome::get_sun_light(void)
{
	// TODO

	light sun;

	sun.type      = FUSE_LIGHT_TYPE_DIRECTIONAL;

	sun.color     = color_rgb(1, 1, 1);
	sun.intensity = 5.f;
	
	sun.ambient   = m_ambientColor * m_ambientIntensity;

	sun.direction = compute_sun_direction(m_zenith, m_azimuth);
	sun.skydome   = this;

	return sun;
}

bool skydome_renderer::init(ID3D12Device * device)
{
	return create_pso(device) && create_nishita_pso(device);
}

void skydome_renderer::shutdown(void)
{
	m_pso.reset();
	m_rs.reset();
}

struct cbSkydome
{

	float thetaSun;
	float cosThetaSun;

	float __fill0[2];

	float3 sunDirection;

	float zenith_Y;
	float zenith_x;
	float zenith_y;

	float __fill1[2];

	float4 perez_Y[5];
	float4 perez_x[5];
	float4 perez_y[5];

};

void skydome_renderer::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	skydome & sky,
	uint32_t flags)
{

	if (!sky.m_uptodate || flags & FUSE_SKYDOME_FLAG_FORCE_UPDATE)
	{

		uint32_t bufferIndex = (sky.m_lastUpdatedBuffer + 1) % sky.m_skydomes.size();
		
		if (render_sky_nishita(device, commandQueue, commandList, ringBuffer, sky, bufferIndex))
		{
			sky.m_lastUpdatedBuffer = bufferIndex;
		}
		
		render_sky_nishita(device, commandQueue, commandList, ringBuffer, sky, bufferIndex);

	}

}

bool skydome_renderer::create_pso(ID3D12Device * device)
{
	com_ptr<ID3DBlob> quadVS, skyboxGS, skyboxPS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[1];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	D3D_SHADER_MACRO quadDefines[] = {
		{ "QUAD_INSTANCE_ID", "" },
		{ "QUAD_VIEW_RAY", "" },
		{ nullptr, nullptr }
	};

	if (compile_shader(FUSE_LITERAL("shaders/quad_vs.hlsl"), quadDefines, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
	    compile_shader(FUSE_LITERAL("shaders/skydome.hlsl"), quadDefines, "skydome_gs", "gs_5_0", compileFlags, &skyboxGS) &&
	    compile_shader(FUSE_LITERAL("shaders/skydome.hlsl"), quadDefines, "skydome_ps", "ps_5_0", compileFlags, &skyboxPS) &&
	    reflect_input_layout(quadVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
	    !FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
	    !FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_rs))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.NumRenderTargets              = 1;
		psoDesc.RTVFormats[0]                 = SKYDOME_FORMAT;
		psoDesc.VS                            = { FUSE_BLOB_ARGS(quadVS) };
		psoDesc.GS                            = { FUSE_BLOB_ARGS(skyboxGS) };
		psoDesc.PS                            = { FUSE_BLOB_ARGS(skyboxPS) };
		psoDesc.InputLayout                   = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                = m_rs.get();
		psoDesc.SampleMask                    = UINT_MAX;
		psoDesc.SampleDesc                    = { 1, 0 };
		psoDesc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

	}

	return false;
}

bool skydome_renderer::create_nishita_pso(ID3D12Device * device)
{

	com_ptr<ID3DBlob> skydomeVS, nishitaSkydomePS;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	com_ptr<ID3DBlob> serializedSignature;
	com_ptr<ID3DBlob> errorsBlob;
	com_ptr<ID3D12ShaderReflection> shaderReflection;

	CD3DX12_ROOT_PARAMETER rootParameters[1];

	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	UINT compileFlags = 0;

	D3D_SHADER_MACRO quadDefines[] = {
		{ "QUAD_INSTANCE_ID", "" },
		{ "QUAD_VIEW_RAY", "" },
		{ nullptr, nullptr }
	};

	if (compile_shader(FUSE_LITERAL("shaders/skydome.hlsl"), quadDefines, "skydome_vs", "vs_5_0", compileFlags, &skydomeVS) &&
		compile_shader(FUSE_LITERAL("shaders/skydome.hlsl"), quadDefines, "skydome_nishita_ps", "ps_5_0", compileFlags, &nishitaSkydomePS) &&
		reflect_input_layout(skydomeVS.get(), std::back_inserter(inputLayoutVector), &shaderReflection) &&
		!FUSE_HR_FAILED_BLOB(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedSignature, &errorsBlob), errorsBlob) &&
		!FUSE_HR_FAILED(device->CreateRootSignature(0, FUSE_BLOB_ARGS(serializedSignature), IID_PPV_ARGS(&m_nishitaSkydomeRS))))
	{

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.BlendState                    = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState               = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState             = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.NumRenderTargets              = 1;
		psoDesc.RTVFormats[0]                 = SKYDOME_FORMAT;
		psoDesc.VS                            = { FUSE_BLOB_ARGS(skydomeVS) };
		psoDesc.PS                            = { FUSE_BLOB_ARGS(nishitaSkydomePS) };
		psoDesc.InputLayout                   = make_input_layout_desc(inputLayoutVector);
		psoDesc.pRootSignature                = m_nishitaSkydomeRS.get();
		psoDesc.SampleMask                    = UINT_MAX;
		psoDesc.SampleDesc                    = { 1, 0 };
		psoDesc.PrimitiveTopologyType         = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		return !FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_nishitaSkydomePSO)));

	}

	return false;

}

static void skydome_nishita_setup(cbSkydome & skydata, float thetaSun, float azimuth, float turbidity)
{

	float theta2 = thetaSun * thetaSun;
	float theta3 = theta2 * thetaSun;

	float T = turbidity;
	float T2 = turbidity * turbidity;

	skydata.thetaSun = thetaSun;
	skydata.cosThetaSun = std::cos(thetaSun);

	skydata.sunDirection = compute_sun_direction(thetaSun, azimuth);

	float chi = (4.f / 9.f - T / 120.f) * (FUSE_PI - 2.f * thetaSun);

	skydata.zenith_Y = (4.0453f * T - 4.9710f) * std::tan(chi) - .2155f * T + 2.4192f;
	//skydata.zenith_Y *= 1000.f;  // conversion from kcd/m^2 to cd/m^2

	skydata.zenith_x =
		(+0.00165f * theta3 - 0.00374f * theta2 + 0.00208f * thetaSun + 0)        * T2 +
		(-0.02902f * theta3 + 0.06377f * theta2 - 0.03202f * thetaSun + 0.00394f) * T +
		(+0.11693f * theta3 - 0.21196f * theta2 + 0.06052f * thetaSun + 0.25885f);

	skydata.zenith_y =
		(+0.00275f * theta3 - 0.00610f * theta2 + 0.00316f * thetaSun + 0)        * T2 +
		(-0.04214f * theta3 + 0.08970f * theta2 - 0.04153f * thetaSun + 0.00515f) * T +
		(+0.15346f * theta3 - 0.26756f * theta2 + 0.06669f * thetaSun + 0.26688f);

	skydata.perez_Y[0].x = 0.17872f * T - 1.46303f;
	skydata.perez_Y[1].x = -0.35540f * T + 0.42749f;
	skydata.perez_Y[2].x = -0.02266f * T + 5.32505f;
	skydata.perez_Y[3].x = 0.12064f * T - 2.57705f;
	skydata.perez_Y[4].x = -0.06696f * T + 0.37027f;

	skydata.perez_x[0].x = -0.01925f * T - 0.25922f;
	skydata.perez_x[1].x = -0.06651f * T + 0.00081f;
	skydata.perez_x[2].x = -0.00041f * T + 0.21247f;
	skydata.perez_x[3].x = -0.06409f * T - 0.89887f;
	skydata.perez_x[4].x = -0.00325f * T + 0.04517f;

	skydata.perez_y[0].x = -0.01669f * T - 0.26078f;
	skydata.perez_y[1].x = -0.09495f * T + 0.00921f;
	skydata.perez_y[2].x = -0.00792f * T + 0.21023f;
	skydata.perez_y[3].x = -0.04405f * T - 1.65369f;
	skydata.perez_y[4].x = -0.01092f * T + 0.05291f;

}

bool skydome_renderer::render_sky_nishita(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	skydome & sky,
	uint32_t bufferIndex)
{

	render_resource & skydomeRT = sky.m_skydomes[bufferIndex];

	D3D12_GPU_VIRTUAL_ADDRESS address;

	if (void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cbSkydome), &address))
	{

		auto rtv = skydomeRT.get_rtv_cpu_descriptor_handle();

		cbSkydome skydata;

		skydome_nishita_setup(skydata, sky.get_zenith(), sky.get_azimuth(), sky.get_turbidity());

		memcpy(cbData, &skydata, sizeof(cbSkydome));

		commandList->SetPipelineState(m_nishitaSkydomePSO.get());

		commandList.resource_barrier_transition(skydomeRT.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ClearRenderTargetView(rtv, color_rgba::zero, 0, nullptr);

		commandList->SetGraphicsRootSignature(m_nishitaSkydomeRS.get());
		commandList->SetGraphicsRootConstantBufferView(0, address);

		commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

		uint2 resolution = sky.get_skydome_resolution();

		commandList->RSSetViewports(1, &make_fullscreen_viewport(resolution.x, resolution.y));
		commandList->RSSetScissorRects(1, &make_fullscreen_scissor_rect(resolution.x, resolution.y));

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 6, 0, 0);

		return true;

	}

	return false;

}