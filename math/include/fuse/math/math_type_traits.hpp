#pragma once

namespace fuse
{

	namespace detail
	{

		template <typename T>
		struct is_matrix :
			std::false_type {};

		template <typename T, int N, int M>
		struct is_matrix<matrix<T, N, M>> :
			std::true_type {};

		template <typename T>
		struct is_row_vector :
			std::integral_constant<bool, is_matrix<T>::value && T::rows::value == 1> {};

		template <typename T>
		struct is_column_vector :
			std::integral_constant<bool, is_matrix<T>::value && T::cols::value == 1> {};

		template <typename T>
		struct is_vector :
			std::integral_constant<bool, is_row_vector<T>::value || is_column_vector<T>::value> {};

	}

}