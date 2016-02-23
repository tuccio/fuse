#include <fuse/compile_shader.hpp>
#include <fuse/directx_helper.hpp>
#include <fuse/logger.hpp>
#include <fuse/string.hpp>

#include <d3dcompiler.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <locale>
#include <codecvt>

using namespace fuse;

namespace fuse
{

	bool compile_shader(const char_t * filename, const D3D_SHADER_MACRO * defines, const char * entryPoint, const char * target, UINT compileOptions, ID3DBlob ** shader)
	{

#ifdef FUSE_COMPILE_SHADER_GLOBAL_OPTIONS

		compileOptions |= FUSE_COMPILE_SHADER_GLOBAL_OPTIONS;

#endif

		std::ifstream in(filename);

		if (!in)
		{
			FUSE_LOG_OPT(FUSE_LITERAL("fuse::compile_shader"), stringstream_t() << "Failed to open file \"" << filename << "\".");
		}
		else
		{

			com_ptr<ID3DBlob> errors;

			if (!FUSE_HR_FAILED(D3DCompileFromFile(string_widen(filename).c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, compileOptions, 0, shader, &errors)))
			{
				return true;			
			}
			else
			{
				stringstream_t ss;
				ss << "While compiling \"" << entryPoint << "\" from \"" << filename << "\":" << std::endl << static_cast<const char *>(errors->GetBufferPointer());
				FUSE_LOG_OPT(FUSE_LITERAL("fuse::compile_shader"), ss);
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

}

