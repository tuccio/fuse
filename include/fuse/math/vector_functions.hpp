#pragma once

namespace fuse
{

	template <typename VectorType>
	std::enable_if_t<detail::is_vector<VectorType>::value, typename VectorType::scalar> dot(const VectorType & a, const VectorType & b)
	{
		return detail::vector_dot_impl<VectorType, VectorType::size::value>::dot(a, b);
	}

	template <typename VectorType>
	std::enable_if_t<detail::is_vector<VectorType>::value, float> length(const VectorType & a)
	{
		return detail::vector_length_impl<VectorType, VectorType::size::value>::length(a);
	}

	template <typename VectorType>
	VectorType normalize(const VectorType & a)
	{
		return a / length(a);
	}

}