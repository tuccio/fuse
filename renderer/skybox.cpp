#include "skybox.hpp"

#include <fuse/compile_shader.hpp>
#include <fuse/pipeline_state.hpp>

using namespace fuse;

#define SKYBOX_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT

bool skybox::init(ID3D12Device * device, uint32_t resolution, uint32_t buffers)
{

	m_uptodate = false;

	m_cubemaps.resize(buffers);

	for (auto & cubemap : m_cubemaps)
	{

		if (cubemap.create(
			device,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				SKYBOX_FORMAT,
				resolution, resolution,
				6, 0, 1, 0,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&CD3DX12_CLEAR_VALUE(SKYBOX_FORMAT, &std::array<float, 4>()[0])))
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

			srvDesc.Format                  = SKYBOX_FORMAT;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels   = -1;

			cubemap.create_render_target_view(device);
			cubemap.create_shader_resource_view(device, &srvDesc);

			cubemap->SetName(L"fuse_skybox_cubemap");

		}
		else
		{
			shutdown();
			return false;
		}

	}

	m_resolution = resolution;

	return true;

}

void skybox::shutdown(void)
{
	m_cubemaps.clear();
}

render_resource & skybox::get_cubemap(void)
{
	return m_cubemaps[m_lastUpdatedBuffer];
}

static XMFLOAT3 compute_sun_direction(float zenith, float azimuth)
{

	float sinAzimuth, cosAzimuth;
	float sinElevation, cosElevation;

	XMScalarSinCos(&sinAzimuth, &cosAzimuth, azimuth);
	XMScalarSinCos(&cosElevation, &sinElevation, zenith); // cos(elevation) = sin(zenith) ..

	XMFLOAT3 direction;

	direction.y = sinElevation;
	direction.x = cosElevation * cosAzimuth;
	direction.z = cosElevation * sinAzimuth;

	return direction;

}

light skybox::get_sun_light(void) const
{

	// TODO

	light sun;

	sun.type      = FUSE_LIGHT_TYPE_DIRECTIONAL;

	sun.color     = color_rgb(1, 1, 1);
	sun.intensity = 5.f;
	
	sun.ambient   = color_rgb(0, 0, 0);

	sun.direction = compute_sun_direction(m_zenith, m_azimuth);

	return sun;

}

/* skybox_renderer */

bool skybox_renderer::init(ID3D12Device * device)
{
	return create_pso(device);
}

void skybox_renderer::shutdown(void)
{
	m_pso.reset();
	m_rs.reset();
}

struct cbSkybox
{

	float thetaSun;
	float cosThetaSun;

	float __fill[2];

	XMFLOAT3 sunDirection;

	float zenith_Y;
	float zenith_x;
	float zenith_y;

	float perez_Y[5];
	float perez_x[5];
	float perez_y[5];

};

void skybox_setup_constant_buffer(cbSkybox & skydata, float thetaSun, float azimuth, float turbidity)
{

	float theta2 = thetaSun * thetaSun;
	float theta3 = theta2 * thetaSun;
	float T = turbidity;
	float T2 = turbidity * turbidity;

	skydata.thetaSun    = thetaSun;
	skydata.cosThetaSun = std::cos(thetaSun);

	skydata.sunDirection = compute_sun_direction(thetaSun, azimuth);

	float chi = (4.f / 9.f - T / 120.f) * (pi<float>() - 2.f * thetaSun);

	skydata.zenith_Y = (4.0453f * T - 4.9710f) * std::tan(chi) - .2155f * T + 2.4192f;
	skydata.zenith_Y *= 1000.f;  // conversion from kcd/m^2 to cd/m^2

	skydata.zenith_x =
		(+0.00165f * theta3 - 0.00374f * theta2 + 0.00208f * thetaSun + 0)        * T2 +
		(-0.02902f * theta3 + 0.06377f * theta2 - 0.03202f * thetaSun + 0.00394f) * T +
		(+0.11693f * theta3 - 0.21196f * theta2 + 0.06052f * thetaSun + 0.25885f);

	skydata.zenith_y =
		(+0.00275f * theta3 - 0.00610f * theta2 + 0.00316f * thetaSun + 0)        * T2 +
		(-0.04214f * theta3 + 0.08970f * theta2 - 0.04153f * thetaSun + 0.00515f) * T +
		(+0.15346f * theta3 - 0.26756f * theta2 + 0.06669f * thetaSun + 0.26688f);

	skydata.perez_Y[0] =  0.17872f * T - 1.46303f;
	skydata.perez_Y[1] = -0.35540f * T + 0.42749f;
	skydata.perez_Y[2] = -0.02266f * T + 5.32505f;
	skydata.perez_Y[3] =  0.12064f * T - 2.57705f;
	skydata.perez_Y[4] = -0.06696f * T + 0.37027f;

	skydata.perez_x[0] = -0.01925f * T - 0.25922f;
	skydata.perez_x[1] = -0.06651f * T + 0.00081f;
	skydata.perez_x[2] = -0.00041f * T + 0.21247f;
	skydata.perez_x[3] = -0.06409f * T - 0.89887f;
	skydata.perez_x[4] = -0.00325f * T + 0.04517f;

	skydata.perez_y[0] = -0.01669f * T - 0.26078f;
	skydata.perez_y[1] = -0.09495f * T + 0.00921f;
	skydata.perez_y[2] = -0.00792f * T + 0.21023f;
	skydata.perez_y[3] = -0.04405f * T - 1.65369f;
	skydata.perez_y[4] = -0.01092f * T + 0.05291f;

}

void skybox_renderer::render(
	ID3D12Device * device,
	gpu_command_queue & commandQueue,
	gpu_graphics_command_list & commandList,
	gpu_ring_buffer & ringBuffer,
	skybox & sky)
{

	if (sky.m_uptodate)
	{
		return;
	}

	uint32_t bufferIndex = (1 + commandQueue.get_frame_index()) % sky.m_cubemaps.size();

	render_resource & cubemap = sky.m_cubemaps[bufferIndex];

	D3D12_GPU_VIRTUAL_ADDRESS address;
	
	if (void * cbData = ringBuffer.allocate_constant_buffer(device, commandQueue, sizeof(cbSkybox), &address))
	{

		auto rtv = cubemap.get_rtv_cpu_descriptor_handle();

		cbSkybox skydata;

		skybox_setup_constant_buffer(skydata, sky.get_zenith(), sky.get_azimuth(), sky.get_turbidity());
		
		memcpy(cbData, &skydata, sizeof(cbSkybox));

		commandList->SetPipelineState(m_pso.get());

		commandList.resource_barrier_transition(cubemap.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

		float black[4] = { 0 };
		commandList->ClearRenderTargetView(rtv, black, 0, nullptr);

		commandList->SetGraphicsRootSignature(m_rs.get());
		commandList->SetGraphicsRootConstantBufferView(0, address);

		commandList->OMSetRenderTargets(1, &rtv, false, nullptr);

		commandList->RSSetViewports(1, &make_fullscreen_viewport(sky.get_resolution(), sky.get_resolution()));
		commandList->RSSetScissorRects(1, &make_fullscreen_scissor_rect(sky.get_resolution(), sky.get_resolution()));

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		commandList->DrawInstanced(4, 6, 0, 0);

		sky.m_lastUpdatedBuffer = bufferIndex;

		//sky.m_uptodate = true;

	}

}

bool skybox_renderer::create_pso(ID3D12Device * device)
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

	if (compile_shader("shaders/quad_vs.hlsl", quadDefines, "quad_vs", "vs_5_0", compileFlags, &quadVS) &&
	    compile_shader("shaders/skybox.hlsl", quadDefines, "skybox_gs", "gs_5_0", compileFlags, &skyboxGS) &&
	    compile_shader("shaders/skybox.hlsl", quadDefines, "skybox_ps", "ps_5_0", compileFlags, &skyboxPS) &&
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
		psoDesc.RTVFormats[0]                 = SKYBOX_FORMAT;
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