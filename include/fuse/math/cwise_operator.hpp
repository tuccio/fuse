//#pragma once
//
//namespace fuse
//{
//
//	// get
//
//	template <int N, typename VectorType>
//	inline std::enable_if_t<is_scalar_vector<VectorType>::value, typename VectorType::scalar &> get(VectorType & x)
//	{
//		return x.m[N];
//	}
//
//	template <int N, typename VectorType>
//	inline std::enable_if_t<is_scalar_vector<VectorType>::value, const typename VectorType::scalar &> get(const VectorType & x)
//	{
//		return x.m[N];
//	}
//
//	template <int N, int M, typename VectorType>
//	inline std::enable_if_t<is_scalar_matrix<VectorType>::value, typename VectorType::scalar &> get(VectorType & x)
//	{
//		return x.m[VectorType::rows::value * M + N];
//	}
//
//	template <int N, int M, typename VectorType>
//	inline std::enable_if_t<is_scalar_matrix<VectorType>::value, const typename VectorType::scalar &> get(const VectorType & x)
//	{
//		return x.m[VectorType::rows::value * M + N];
//	}
//
//	// operators
//
//	namespace detail
//	{
//
//		template <int N, typename VectorType>
//		inline std::enable_if_t<is_scalar_matrix<VectorType>::value, typename VectorType::scalar &> get(VectorType & x)
//		{
//			return x.m[N];
//		}
//
//		template <int N, typename VectorType>
//		inline std::enable_if_t<is_scalar_matrix<VectorType>::value, const typename VectorType::scalar &> get(const VectorType & x)
//		{
//			return x.m[N];
//		}
//
//		template <typename VectorType, int N = VectorType::size::value>
//		struct cwise_operator
//		{
//
//			inline static void add(VectorType & r, const VectorType & a, const VectorType & b)
//			{
//				cwise_operator<VectorType, N - 1>::add(r, a, b);
//				get<N - 1>(r) = get<N - 1>(a) + get<N - 1>(b);
//			}
//
//			inline static void subtract(VectorType & r, const VectorType & a, const VectorType & b)
//			{
//				cwise_operator<VectorType, N - 1>::subtract(r, a, b);
//				get<N - 1>(r) = get<N - 1>(a) - get<N - 1>(b);
//			}
//
//			inline static void multiply(VectorType & r, const VectorType & a, const VectorType & b)
//			{
//				cwise_operator<VectorType, N - 1>::multiply(r, a, b);
//				get<N - 1>(r) = get<N - 1>(a) * get<N - 1>(b);
//			}
//
//			inline static void divide(VectorType & r, const VectorType & a, const VectorType & b)
//			{
//				cwise_operator<VectorType, N - 1>::divide(r, a, b);
//				get<N - 1>(r) = get<N - 1>(a) / get<N - 1>(b);
//			}
//
//			inline static void negate(VectorType & r, const VectorType & a)
//			{
//				cwise_operator<VectorType, N - 1>::negate(r, a);
//				get<N - 1>(r) = -get<N - 1>(a);
//			}
//
//			inline static void hadd(typename VectorType::scalar & r, const VectorType & a)
//			{
//				cwise_operator<VectorType, N - 1>::hadd(r, a);
//				r += get<N - 1>(a);
//			}
//
//			inline static void absolute(VectorType & r, const VectorType & a)
//			{
//				cwise_operator<VectorType, N - 1>::absolute(r, a);
//				get<N - 1>(r) = fuse::absolute(get<N - 1>(a));
//			}
//
//		};
//
//		template <typename VectorType>
//		struct cwise_operator<VectorType, 0>
//		{
//			inline static void add(VectorType & r, const VectorType & a, const VectorType & b) {}
//			inline static void subtract(VectorType & r, const VectorType & a, const VectorType & b) {}
//			inline static void multiply(VectorType & r, const VectorType & a, const VectorType & b) {}
//			inline static void divide(VectorType & r, const VectorType & a, const VectorType & b) {}
//			inline static void negate(VectorType & r, const VectorType & a) {}
//			inline static void absolute(VectorType & r, const VectorType & a) {}
//		};
//
//	}
//
//	template <typename VectorType>
//	inline VectorType add(const VectorType & a, const VectorType & b)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::add(r, a, b);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline VectorType subtract(const VectorType & a, const VectorType & b)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::subtract(r, a, b);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline VectorType scale(const VectorType & a, const VectorType & b)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::multiply(r, a, b);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline VectorType divide(const VectorType & a, const VectorType & b)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::divide(r, a, b);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline VectorType negate(const VectorType & a)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::negate(r, a);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline VectorType absolute(const VectorType & a)
//	{
//		VectorType r;
//		detail::cwise_operator<VectorType>::absolute(r, a);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline typename VectorType::scalar hadd(const VectorType & a)
//	{
//		VectorType::scalar r = 0;
//		detail::cwise_operator<VectorType>::hadd(r, a);
//		return r;
//	}
//
//	template <typename VectorType>
//	inline bool equals(typename VectorType::scalar a, const VectorType & b)
//	{
//		assert(sizeof(VectorType) == sizeof(VectorType::scalar) * sizeof(VectorType::size::value) &&
//			"Padding detected, can't use memcmp to compare objects.");
//		return memcmp(&a, &b, sizeof(VectorType)) == 0;
//	}
//
//}