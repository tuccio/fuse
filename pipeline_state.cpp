#include <fuse/pipeline_state.hpp>
#include <fuse/compile_shader.hpp>
#include <fuse/string.hpp>

#include <algorithm>
#include <sstream>

using namespace fuse;

pipeline_state_template::pipeline_state_template(
	std::initializer_list<shader_macro_domain> macros,
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC & psoDesc,
	const char * compilerTarget,
	UINT compileFlags,
	const D3D_SHADER_MACRO * commonDefines) :
	m_psoDesc(psoDesc),
	m_compilerFlags(compileFlags),
	m_compilerTarget(compilerTarget),
	m_macros(macros)
{

	std::sort(m_macros.begin(), m_macros.end(),
		[](const shader_macro_domain & a, const shader_macro_domain & b)
	{
		return std::less<std::string>()(a.get_macro(), b.get_macro());
	});

	if (commonDefines)
	{

		for (auto it = commonDefines; it->Name; it++)
		{
			m_commonDefines.emplace_back(it->Name, it->Definition);
		}

	}

	create_hash_offsets();
	
}

void pipeline_state_template::clear(void)
{
	swap(pipeline_state_template());
}

void pipeline_state_template::swap(pipeline_state_template && pst)
{
	pipeline_state_template tmp = std::forward<pipeline_state_template>(pst);
	pst = std::move(*this);
	*this = std::move(tmp);
}

ID3D12PipelineState * pipeline_state_template::compile_permutation(ID3D12Device * device, std::initializer_list<D3D_SHADER_MACRO> defines)
{

	boost::optional<size_t> hash = compute_hash(defines);

	if (!hash)
	{
		return nullptr;
	}

	auto it = m_psos.find(*hash);

	if (it != m_psos.end())
	{
		return it->second.get();
	}

	std::vector<D3D_SHADER_MACRO> definesVector(defines.size() + m_commonDefines.size() + 1);

	std::transform(m_commonDefines.begin(), m_commonDefines.end(), definesVector.begin(),
		[](const std::pair<std::string, std::string> & define)
	{
		return D3D_SHADER_MACRO{ define.first.c_str(), define.second.c_str() };
	});

	std::copy(defines.begin(), defines.end(), std::next(definesVector.begin(), m_commonDefines.size()));

	com_ptr<ID3DBlob> vertexShader, pixelShader;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutVector;

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {}, pixelShaderBytecode = {};

	com_ptr<ID3D12ShaderReflection> shaderReflection;

	if (m_vertexShader)
	{

		if (compile_shader(m_vertexShader->filename.c_str(), &definesVector[0], m_vertexShader->entrypoint.c_str(), ("vs_" + m_compilerTarget).c_str(), m_compilerFlags, &vertexShader) &&
			reflect_input_layout(vertexShader.get(), std::back_inserter(inputLayoutVector), &shaderReflection))
		{

			if (m_ilvFunctor)
			{
				std::for_each(inputLayoutVector.begin(), inputLayoutVector.end(), m_ilvFunctor);
			}

			vertexShaderBytecode = make_shader_bytecode(vertexShader.get());

		}

	}

	if (m_pixelShader)
	{
		if (compile_shader(m_pixelShader->filename.c_str(), &definesVector[0], m_pixelShader->entrypoint.c_str(), ("ps_" + m_compilerTarget).c_str(), m_compilerFlags, &pixelShader))
		{
			pixelShaderBytecode = make_shader_bytecode(pixelShader.get());
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = m_psoDesc;

	psoDesc.InputLayout = make_input_layout_desc(inputLayoutVector);

	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;

	com_ptr<ID3D12PipelineState> pso;

	if (!FUSE_HR_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso))))
	{
		m_psos.emplace(*hash, pso);
		return pso.get();
	}

	return nullptr;

}

boost::optional<size_t> pipeline_state_template::compute_hash(std::initializer_list<D3D_SHADER_MACRO> defines)
{

	size_t hash = 0;

	for (auto & define : defines)
	{

		// Binary search on the macro definitions to find the macro index
		// we use to get the bit offset in the hash built in create_hash_offsets

		auto it = std::lower_bound(m_macros.begin(), m_macros.end(), define.Name,
			[](const shader_macro_domain & a, const char * b)
		{
			return std::less<std::string>()(a.get_macro(), b);
		});

		if (it == m_macros.end() || it->get_macro() != define.Name)
		{
			FUSE_LOG_DEBUG(stringstream_t() << "Unrecognized macro definition: \"" << define.Name << "\".");
			return {};
		}
		else
		{
			
			size_t index = std::distance(m_macros.begin(), it);
			size_t offset = index == 0 ? 0 : m_hashOffsets[index - 1];

			auto & domain = it->get_domain();

			// Binary search on the domain to find if the value is valid
			auto valueIt = std::lower_bound(domain.begin(), domain.end(), define.Definition);
			
			if (*valueIt == define.Definition)
			{
				// Add 1 cause value 0 is for undefined
				size_t value = std::distance(domain.begin(), valueIt) + 1;
				hash |= value << offset;
			}
			else
			{
				FUSE_LOG_DEBUG(stringstream_t() << "Unrecognized value for macro \"" << define.Name << "\": \"" << define.Definition << "\".");
				return {};
			}

		}

	}

	return hash;

}

bool pipeline_state_template::create_hash_offsets(void)
{

	// Creates a vector of partial sums of bits allocation, that
	// allows the computation of offsets for hashing in O(1)

	m_hashOffsets.clear();
	m_hashOffsets.reserve(m_macros.size());

	size_t currentOffset = 0;

	for (auto & macro : m_macros)
	{

		// Value 0 is for undefined, so we add 1 to the domain size
		size_t domainSize = 1 + macro.get_domain().size();
		size_t bits = (size_t) std::ceil(std::log2(domainSize));

		currentOffset += bits;
		m_hashOffsets.push_back(currentOffset);

	}

	if (currentOffset > sizeof(size_t) * 8)
	{
		FUSE_LOG_DEBUG(stringstream_t() << "Macro permutations hash values out of range.");
		return false;
	}

	return true;

}