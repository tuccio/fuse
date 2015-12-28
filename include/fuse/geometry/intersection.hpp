#pragma once

#include <type_traits>
#include <utility>

namespace fuse
{

	namespace detail
	{

		template <typename T, typename U>
		struct intersection_impl
		{
			typedef std::false_type implemented;
		};

		template <typename T, typename U>
		struct intersection_base
		{
			typedef std::true_type implemented;
		};

		template <typename T, typename U>
		struct is_intersectable :
			std::is_same<typename intersection_impl<T, U>::implemented, std::true_type> { };

	}

	template <typename T, typename U>
	inline bool contains(const T & a, const U & b)
	{
		static_assert(detail::intersection_impl<T, U>::implemented::value, "fuse::contains: No available implementation for given volumes (check template types for more information).");
		return detail::intersection_impl<T, U>::contains(a, b);
	}

	/* Intersection test functions */

	template <typename T, typename U, typename ... OptionalArguments>
	inline std::enable_if_t<detail::is_intersectable<T, U>::value, bool>
		intersects(const T & a, const U & b, OptionalArguments && ... args)
	{
		return detail::intersection_impl<T, U>::intersects(a, b, std::forward<OptionalArguments>(args) ...);
	}

	template <typename T, typename U, typename ... OptionalArguments>
	inline std::enable_if_t<!std::is_same<T, U>::value && detail::is_intersectable<U, T>::value, bool>
		intersects(const T & a, const U & b, OptionalArguments && ... args)
	{
		return detail::intersection_impl<U, T>::intersects(b, a, std::forward<OptionalArguments>(args) ...);
	}

	template <typename T, typename U, typename ... OptionalArguments>
	inline
		typename std::enable_if_t<!detail::is_intersectable<T, U>::value && !detail::is_intersectable<U, T>::value, bool>
		intersects(const T &, const U &, OptionalArguments && ...)
	{
		static_assert(false, "fuse::intersection: No available implementation for given volumes (check template types for more information).");
		return false;
	}

}

#include "intersection.inl"