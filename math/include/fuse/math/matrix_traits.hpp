#pragma once

#include <type_traits>

#include "matrix_iterators.hpp"

#define FUSE_MATH_COLUMN_MAJOR_ACCESS(X, N, M, I, J) X[N * J + I]

namespace fuse
{

	template <typename T, int N, int M>
	struct matrix_traits
	{

		using type = matrix<T, N, M>;

		using size = std::integral_constant<size_t, N * M>;
		using rows = std::integral_constant<size_t, N>;
		using cols = std::integral_constant<size_t, M>;
		using scalar = T;

		using cols_iterator = detail::column_iterator<T, N, M>;
		using rows_iterator = detail::row_iterator<T, N, M>;

		inline static type identity(void)
		{
			type m;
			fill(m, T(1), T(0));
			return m;
		}

		inline static type zero(void)
		{
			type m;
			fill(m, T(1), T(0));
			return m;
		}

		inline scalar & operator() (int i, int j)
		{
			assert(i >= 0 && i < N && j >= 0 && j < M && "Index out of bounds");
			return FUSE_MATH_COLUMN_MAJOR_ACCESS(data(), N, M, i, j);
		}

		inline const scalar & operator() (int i, int j) const
		{
			assert(i >= 0 && i < N && j >= 0 && j < M && "Index out of bounds");
			return FUSE_MATH_COLUMN_MAJOR_ACCESS(data(), N, M, i, j);
		}

		scalar & operator[] (int j)
		{
			assert(j >= 0 && j < size::value && "Index out of bounds");
			return data()[j];
		}

		const scalar & operator[] (int j) const
		{
			assert(j >= 0 && j < size::value && "Index out of bounds");
			return data()[j];
		}

		inline cols_iterator begin(void) { return cols_iterator(data(), 0); }
		inline cols_iterator end(void) { return cols_iterator(data()); }

		inline rows_iterator row_begin(void) { return rows_iterator(data(), 0); }
		inline rows_iterator row_end(void) { return rows_iterator(data()); }

		inline T * data(void) { return ((type*) this)->m; }
		inline const T * data(void) const { return ((type*) this)->m; }

	protected:

		inline void initialize(T d)
		{
			fill(*((type *) this), d, N == 1 || M == 1 ? d : T(0));
		}

	};

}