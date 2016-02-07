#include <fuse/render_resource_manager.hpp>
#include <fuse/logger.hpp>

#include <algorithm>
#include <sstream>

using namespace fuse;
using namespace fuse::detail;

#define FUSE_RENDER_RESOURCE_INVALID_ID ((UINT) (-1))

namespace std
{

	template <>
	struct less<render_resource_wrapper *>
	{
		bool operator() (const render_resource_wrapper * lhs, const render_resource_wrapper * rhs)
		{
			return lhs->bufferIndex < rhs->bufferIndex &&
				(UINT)lhs->description.Format < (UINT)rhs->description.Format &&
				lhs->description.Width < rhs->description.Width &&
				lhs->description.Height < rhs->description.Height;
		}
	};

}

render_resource_id_t render_resource_manager::find_texture_2d(UINT bufferIndex, const D3D12_RESOURCE_DESC & description)
{

	assert(description.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

	render_resource_wrapper resourceWrapper = { description, bufferIndex, 0, false };

	auto eqr = std::equal_range(m_descriptionMap.begin(), m_descriptionMap.end(), &resourceWrapper, std::less<render_resource_wrapper *>());

	for (auto it = eqr.first; it != eqr.second; it++)
	{

		render_resource_wrapper * pIt = (*it);

		if (pIt->usageFlag == false &&
			description.Width == pIt->description.Width &&
			description.Height == pIt->description.Height &&
			(description.Flags & pIt->description.Flags) == description.Flags &&
			description.SampleDesc.Count == pIt->description.SampleDesc.Count &&
			description.SampleDesc.Quality == pIt->description.SampleDesc.Quality &&
			description.DepthOrArraySize == pIt->description.DepthOrArraySize &&
			description.MipLevels == pIt->description.MipLevels)
		{
			return pIt->id;
		}

	}

	return FUSE_RENDER_RESOURCE_INVALID_ID;

}

render_resource_id_t render_resource_manager::create_texture_2d(ID3D12Device * device, UINT bufferIndex, const D3D12_RESOURCE_DESC & description, const D3D12_CLEAR_VALUE * clearValue)
{

	render_resource_id_t id = m_resources.size();

	render_resource_wrapper resourceWrapper = { description, bufferIndex, id, false };

	auto it = std::upper_bound(m_descriptionMap.begin(), m_descriptionMap.end(), &resourceWrapper, std::less<render_resource_wrapper *>());

	render_resource * resource = new render_resource;

	if (!resource->create(device, &description, D3D12_RESOURCE_STATE_COMMON, clearValue))
	{
		delete resource;
		return FUSE_RENDER_RESOURCE_INVALID_ID;
	}

	render_resource_wrapper * pResourceWrapper = new render_resource_wrapper(resourceWrapper);

	m_resources.emplace_back(resource, pResourceWrapper);
	it = m_descriptionMap.insert(it, pResourceWrapper);

	return id;

}

render_resource_ptr render_resource_manager::get_texture_2d(
	ID3D12Device * device,
	UINT bufferIndex,
	DXGI_FORMAT format,
	UINT width, UINT height,
	UINT arraySize,
	UINT mipLevels,
	UINT sampleCount,
	UINT sampleQuality,
	D3D12_RESOURCE_FLAGS flags,
	const D3D12_CLEAR_VALUE * clearValue,
	const D3D12_RENDER_TARGET_VIEW_DESC * rtvDesc,
	const D3D12_SHADER_RESOURCE_VIEW_DESC * srvDesc,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC * uavDesc,
	const D3D12_DEPTH_STENCIL_VIEW_DESC * dsvDesc)
{

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, arraySize, mipLevels, sampleCount, sampleQuality, flags);

	render_resource_id_t id = find_texture_2d(bufferIndex, desc);

	if (id == FUSE_RENDER_RESOURCE_INVALID_ID)
	{
		
		id = create_texture_2d(device, bufferIndex, desc, clearValue);

		if (id == FUSE_RENDER_RESOURCE_INVALID_ID)
		{
			return render_resource_ptr();
		}

		if (!(flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
		{
			m_resources[id].first->create_shader_resource_view(device, srvDesc);
		}

		if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			m_resources[id].first->create_render_target_view(device, rtvDesc);
		}

		if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
		{
			m_resources[id].first->create_unordered_access_view(device, uavDesc);
		}

		if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			m_resources[id].first->create_depth_stencil_view(device, dsvDesc);
		}

	}

	m_resources[id].second->usageFlag = true;
	return render_resource_ptr(m_resources[id].first, m_resources[id].second->id);

}

void render_resource_manager::release_texture_2d(render_resource_id_t id)
{
	m_resources[id].second->usageFlag = false;	
}

void render_resource_manager::clear(void)
{
	
#ifdef _DEBUG
	
	// Check for dangling pointers

	for (auto & r : m_resources)
	{

		if (r.second->usageFlag)
		{

			FUSE_LOG_OPT(FUSE_LITERAL("render_resource_manager::clear"),
				stringstream_t()
				<< "A render resource is still in use when clear() is called (Format:"
				<< r.second->description.Format
				<< ", Size: " << r.second->description.Width << "x" << r.second->description.Height << ").");

		}

	}

#endif

	for (auto & r : m_resources)
	{
		delete r.first;
	}

	m_resources.clear();
	m_descriptionMap.clear();

}

render_resource_handle::~render_resource_handle(void)
{
	reset();
}

void render_resource_handle::reset(void)
{
	if (m_resource)
	{
		render_resource_manager::get_singleton_pointer()->release_texture_2d(m_id);
		m_resource = nullptr;
		m_id = (UINT)-1;
	}
	
}