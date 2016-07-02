#pragma once

#include "math_fwd.hpp"
#include "matrix.hpp"

namespace fuse
{

	template <typename T>
	struct matrix<T, 1, 3> :
		matrix_traits<T, 1, 3>
	{

		matrix(void) = default;
		matrix(const matrix &) = default;

		matrix(scalar value) :
			x(value), y(value), z(value) {}

		matrix(scalar x, scalar y, scalar z) :
			x(x), y(y), z(z) {}

		union
		{

			struct
			{
				scalar x;
				scalar y;
				scalar z;
			};

			scalar m[3];

		};

	};

}

#include "vector3.inl"