#pragma once

#include <d3d12.h>
#include <d3d12shader.h>

#include <d3dcompiler.h>

#include <fuse/directx_helper.hpp>

#include <iterator>
#include <vector>

namespace fuse
{

	bool compile_shader(const char * filename, const D3D_SHADER_MACRO * defines, const char * entryPoint, const char * target, UINT compileOptions, ID3DBlob ** shader);
	
	DXGI_FORMAT get_dxgi_format(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE mask);

	template <template <typename> typename OutputIterator, template <typename, typename ...> typename Container, typename ... Types>
	bool reflect_input_layout(ID3DBlob * vertexShader, OutputIterator<Container<D3D12_INPUT_ELEMENT_DESC, Types ...>> && it, ID3D12ShaderReflection ** shaderReflection)
	{

		if (!FUSE_HR_FAILED(D3DReflect(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_PPV_ARGS(shaderReflection))))
		{

			D3D12_SHADER_DESC shaderDesc;
			(*shaderReflection)->GetDesc(&shaderDesc);

			for (int i = 0; i < shaderDesc.InputParameters; i++)
			{

				D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
				(*shaderReflection)->GetInputParameterDesc(i, &inputParamDesc);

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

	}

}