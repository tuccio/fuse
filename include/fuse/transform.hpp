#pragma once

#include <fuse/math.hpp>

namespace fuse
{

	// TODO

	class transform
	{

	public:

		transform(void);


	private:

		XMVECTOR m_translation;
		XMVECTOR m_orientation;

		mutable XMMATRIX m_world;

	};

}