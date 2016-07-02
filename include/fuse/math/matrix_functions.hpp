#pragma once

namespace fuse
{

	template <int I, typename T, int N, int M>
	T & get(matrix<T, N, M> & x)
	{
		return x.m[I];
	}

	template <int I, typename T, int N, int M>
	const T & get(const matrix<T, N, M> & x)
	{
		return x.m[I];
	}

	template <int I, int J, typename T, int N, int M>
	T & get(matrix<T, N, M> & x)
	{
		return FUSE_MATH_COLUMN_MAJOR_ACCESS(x.m, N, M, I, J);
	}

	template <int I, int J, typename T, int N, int M>
	const T & get(const matrix<T, N, M> & x)
	{
		return FUSE_MATH_COLUMN_MAJOR_ACCESS(x.m, N, M, I, J);
	}

	/* Functions */

	template <typename T, int N, int M>
	void fill(matrix<T, N, M> & m, T d, T f = 0)
	{
		detail::matrix_fill_impl<matrix<T, N, M>, N, M>::fill(m, d, f);
	}

	template <typename T, int N, int M>
	matrix<T, N, M> scale(const matrix<T, N, M> & a, const matrix<T, N, M> & b)
	{
		return detail::matrix_scale_impl<T, N, M>::scale(a, b);
	}

	template <typename T, int N, int M>
	matrix<T, M, N> transpose(const matrix<T, N, M> & a)
	{
		return detail::matrix_transpose_impl<T, N, M>::transpose(a);
	}

}