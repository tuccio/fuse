#pragma once

#include <algorithm>
#include <cassert>
#include <type_traits>

#include "math_fwd.hpp"
#include "utility.hpp"

/* Template matrices */

#include "matrix_iterators.hpp"

#include "matrix_traits.hpp"
#include "matrix.hpp"

#include "math_type_traits.hpp"

#include "scalar_functions.hpp"

#include "cwise_operator.hpp"

#include "scalar_matrix_operators.hpp"
#include "scalar_matrix_functions.hpp"

#include "matrix_functions.hpp"
#include "matrix_operators.hpp"

#include "vector_functions.hpp"

#include "matrix_base_impl.inl"
#include "vector_base_impl.inl"

/* Specializations */

#include "vector2.hpp"
#include "vector3.hpp"
#include "vector4.hpp"

#include "matrix2x2.hpp"
#include "matrix3x3.hpp"
#include "matrix4x4.hpp"

#include "quaternion.hpp"

#include "vec128.hpp"
#include "mat128.hpp"

/* Implementations */

#include "vector3_impl.inl"
#include "vector4_impl.inl"

#include "matrix3x3_impl.inl"
#include "matrix4x4_impl.inl"

#include "quaternion_impl.inl"

#include "vec128_impl.inl"
#include "quat128_impl.inl"
#include "mat128_impl.inl"