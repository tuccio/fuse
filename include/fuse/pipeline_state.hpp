#pragma once

#include <d3d12.h>

namespace fuse
{

	inline D3D12_SHADER_BYTECODE make_shader_bytecode(ID3DBlob * blob)
	{
		return D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() };
	}

	template <template <typename, typename ...> typename Container, typename ... Types>
	D3D12_INPUT_LAYOUT_DESC make_input_layout_desc(const Container<D3D12_INPUT_ELEMENT_DESC, Types ...> & v)
	{
		return D3D12_INPUT_LAYOUT_DESC{ v.empty() ? nullptr : &v.front(), v.size() };
	}

	template <int N>
	D3D12_INPUT_LAYOUT_DESC make_input_layout_desc(const D3D12_INPUT_ELEMENT_DESC (&v)[N])
	{
		return D3D12_INPUT_LAYOUT_DESC{ v, N };
	}

}