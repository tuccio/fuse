#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>
#include <fuse/properties_macros.hpp>

#include <type_traits>

namespace fuse
{

	class alignas(16) sphere
	{

	public:

		sphere(void) = default;
		sphere(const sphere &) = default;
		sphere(sphere &&) = default;

		sphere(const XMFLOAT3 & center, float radius);
		sphere(const XMVECTOR & sphere);

		void set_center(const XMVECTOR & center);

		float get_radius(void) const;
		void set_radius(float radius);

		inline XMVECTOR get_splat_radius(void) const { return XMVectorSplatW(m_sphere); }

		sphere & operator= (const sphere &) = default;
		sphere & operator= (sphere &&) = default;

	private:

		XMVECTOR m_sphere;

	public:

		FUSE_DECLARE_ALIGNED_ALLOCATOR_NEW(16)

		FUSE_PROPERTIES_BY_CONST_REFERENCE(
			(sphere_vector, m_sphere)
		)

		FUSE_PROPERTIES_BY_CONST_REFERENCE_READ_ONLY(
			(center, m_sphere)
		)

	};

}