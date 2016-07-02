#pragma once

#include "math_fwd.hpp"
#include "matrix.hpp"

namespace fuse
{

	template <typename T>
	struct matrix<T, 1, 4> :
		matrix_traits<T, 1, 4>
	{

		matrix(void) = default;
		matrix(const matrix &) = default;

		matrix(scalar value) :
			x(value), y(value), z(value), w(value) {}

		matrix(scalar x, scalar y, scalar z, scalar w) :
			x(x), y(y), z(z), w(w) {}

		union
		{

			struct
			{
				scalar x;
				scalar y;
				scalar z;
				scalar w;
			};

			scalar m[4];

		};

	};

}