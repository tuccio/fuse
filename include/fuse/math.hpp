#pragma once

#include <DirectXMath.h>
#include <boost/math/constants/constants.hpp>

#include <cmath>

namespace fuse
{
	
	using namespace DirectX;

	using namespace boost::math::constants;

	inline XMVECTOR rotate(const XMVECTOR & q, const XMVECTOR & v) { return q * v * XMQuaternionConjugate(q); }
	inline XMVECTOR rotate(const XMVECTOR & q, const XMFLOAT3 & v) { return q * XMLoadFloat3(&v) * XMQuaternionConjugate(q); }

	inline XMFLOAT2 to_float2(const XMVECTOR & q) { return XMFLOAT2(XMVectorGetX(q), XMVectorGetY(q)); }
	inline XMFLOAT2 to_float2(const XMFLOAT2 & v) { return v; }

	inline XMFLOAT3 to_float3(const XMVECTOR & q) { return XMFLOAT3(XMVectorGetX(q), XMVectorGetY(q), XMVectorGetZ(q)); }
	inline XMFLOAT3 to_float3(const XMFLOAT3 & v) { return v; }

	inline XMFLOAT4 to_float4(const XMVECTOR & q) { return XMFLOAT4(XMVectorGetX(q), XMVectorGetY(q), XMVectorGetZ(q), XMVectorGetW(q)); }
	inline XMFLOAT4 to_float4(const XMFLOAT4 & v) { return v; }

	inline XMVECTOR to_vector(const XMFLOAT2 & v) { return XMLoadFloat2(&v); }
	inline XMVECTOR to_vector(const XMFLOAT3 & v) { return XMLoadFloat3(&v); }
	inline XMVECTOR to_vector(const XMFLOAT4 & v) { return XMLoadFloat4(&v); }
	inline XMVECTOR to_vector(const XMVECTOR & v) { return v; }

}