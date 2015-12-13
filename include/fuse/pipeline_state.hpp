#pragma once

#include <fuse/directx_helper.hpp>
#include <fuse/properties_macros.hpp>

#include <boost/optional.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

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

	class shader_macro_domain
	{

	public:

		shader_macro_domain(void) = default;
		shader_macro_domain(const shader_macro_domain &) = default;
		shader_macro_domain(shader_macro_domain &&) = default;

		shader_macro_domain(std::string macro, std::initializer_list<std::string> domain) :
			m_macro(macro), m_domain(domain)
		{
			std::sort(m_domain.begin(), m_domain.end());
		}

		shader_macro_domain(std::string macro) :
			m_macro(macro), m_domain({ "" }) { }

		shader_macro_domain & operator= (const shader_macro_domain &) = default;
		shader_macro_domain & operator= (shader_macro_domain &&) = default;

	private:
		
		std::string              m_macro;
		std::vector<std::string> m_domain;

	public:

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(macro,  m_macro)
			(domain, m_domain)
		)

	};

	namespace detail
	{

		

	}

	class pipeline_state_template
	{

	public:

		pipeline_state_template(void) = default;

		pipeline_state_template(
			std::initializer_list<shader_macro_domain> macros,
			const D3D12_GRAPHICS_PIPELINE_STATE_DESC & psoDesc,
			const char * compilerTargets,
			UINT compileFlags = 0,
			const D3D_SHADER_MACRO * commonDefines = nullptr);

		pipeline_state_template(pipeline_state_template &&) = default;

		void clear(void);

		inline ID3D12PipelineState * get_permutation(ID3D12Device * device, std::initializer_list<D3D_SHADER_MACRO> macros = {})
		{
			return compile_permutation(device, macros);
		}

		inline D3D12_GRAPHICS_PIPELINE_STATE_DESC & get_pso_desc(void) { return m_psoDesc; }

		inline void set_vertex_shader(const char * filename, const char * entrypoint) { m_vertexShader = shader_entry_point { filename, entrypoint }; }
		inline void set_pixel_shader(const char * filename, const char * entrypoint) { m_pixelShader = shader_entry_point{ filename, entrypoint }; }

		template <typename Function>
		inline void set_ilv_functor(Function && f) { m_ilvFunctor = f; }

		void swap(pipeline_state_template && pst);

		pipeline_state_template & operator= (pipeline_state_template &&) = default;

	private:

		struct shader_entry_point
		{
			std::string filename;
			std::string entrypoint;
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc;
		UINT m_compilerFlags;

		std::unordered_map<size_t, com_ptr<ID3D12PipelineState>> m_psos;
		std::vector<shader_macro_domain> m_macros;

		std::vector<std::pair<std::string, std::string>> m_commonDefines;

		boost::optional<shader_entry_point> m_vertexShader;
		boost::optional<shader_entry_point> m_pixelShader;

		std::string m_compilerTarget;

		std::vector<size_t> m_hashOffsets;

		std::function<void(D3D12_INPUT_ELEMENT_DESC &)> m_ilvFunctor;

		ID3D12PipelineState * compile_permutation(ID3D12Device * device, std::initializer_list<D3D_SHADER_MACRO> defines);

		boost::optional<size_t> compute_hash(std::initializer_list<D3D_SHADER_MACRO> defines);
		bool create_hash_offsets(void);

	public:

		FUSE_PROPERTIES_STRING (
			(compiler_target, m_compilerTarget)
		)

	};

}