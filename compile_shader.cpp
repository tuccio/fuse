#include <fuse/compile_shader.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/logger.hpp>>

#include <d3dcompiler.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

using namespace fuse;

namespace fuse
{

	bool compile_shader(const char * filename, const D3D_SHADER_MACRO * defines, const char * entryPoint, const char * target, UINT compileOptions, ID3DBlob ** shader)
	{

		std::ifstream in(filename);

		if (!in)
		{
			FUSE_LOG_OPT("fuse::compile_shader", std::stringstream() << "Failed to open file \"" << filename << "\".");
		}
		else
		{

			std::string data = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

			com_ptr<ID3DBlob> errors;

			if (!FUSE_HR_FAILED(D3DCompile(&data[0], data.size(), filename, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, compileOptions, 0, shader, &errors)))
			{
				return true;
			}
			else
			{
				const char * msg = static_cast<const char *>(errors->GetBufferPointer());
				FUSE_LOG_OPT("fuse::compile_shader", msg);
			}

		}

		return false;

	}

	DXGI_FORMAT get_dxgi_format(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE mask)
	{

		DXGI_FORMAT format;

		if (mask == 1)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (mask <= 3)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (mask <= 7)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (mask <= 15)
		{
			if (componentType == D3D_REGISTER_COMPONENT_UINT32) format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (componentType == D3D_REGISTER_COMPONENT_SINT32) format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		return format;

	}

	/*bool reflect_input_layout(ID3DBlob * vertexShader, std::insert_iterator<D3D12_INPUT_ELEMENT_DESC> && it)
	{

		fuse::com_ptr<ID3D12ShaderReflection> shaderReflection;

		if (!FUSE_HR_FAILED(D3DReflect(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(&shaderReflection))))
		{

			D3D12_SHADER_DESC shaderDesc;
			shaderReflection->GetDesc(&shaderDesc);

			for (int i = 0; i < shaderDesc.InputParameters; i++)
			{

				D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
				shaderReflection->GetInputParameterDesc(i, &inputParamDesc);

				D3D12_INPUT_ELEMENT_DESC elementDesc = { };

				elementDesc.SemanticName      = inputParamDesc.SemanticName;
				elementDesc.SemanticIndex     = inputParamDesc.SemanticIndex;
				elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				elementDesc.InputSlotClass    = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
				elementDesc.Format            = get_dxgi_format(inputParamDesc.ComponentType, inputParamDesc.Mask);

				*it = elementDesc;
				++it;

			}

			return true;

		}

		return false;

	}*/


}

