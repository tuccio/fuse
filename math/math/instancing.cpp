#include <fuse/math.hpp>

using namespace fuse;

template struct matrix<float, 2, 2>;
template struct matrix<float, 3, 3>;
template struct matrix<float, 4, 4>;

template struct matrix<float, 1, 2>;
template struct matrix<float, 1, 3>;
template struct matrix<float, 1, 4>;

template struct matrix<float, 2, 1>;
template struct matrix<float, 3, 1>;
template struct matrix<float, 4, 1>;

//template <float, 3, 3> matrix<float, 3, 3> operator+ (const matrix<float, 3, 3> &, const matrix<float 3, 3> &);

// TODO: operator instancing