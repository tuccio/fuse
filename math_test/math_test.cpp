#include <fuse/math/math.hpp>
#include <fuse/math/to_string.hpp>

#include <Eigen/Eigen>

#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <tuple>

using namespace fuse;

static std::ofstream g_log;

std::ostream & operator<< (std::ostream & os, const Eigen::Quaternionf & q)
{
	quaternion r(q.w(), q.x(), q.y(), q.z());
	return os << r;
}

void test_print_all(std::ostream & os) {}

template <typename Arg1, typename ... Args>
void test_print_all(std::ostream & os, Arg1 && arg1, Args && ... args)
{
	os << "\t" << arg1 << std::endl;
	test_print_all(os, args ...);
}

#define TEST_SUCCESS_LOG(OutputStream)\
{\
	OutputStream << "Test " << __FUNCSIG__ << " OK" << std::endl;\
}

#define TEST_FAIL_LOG(OutputStream, Iteration, ...)\
{\
	OutputStream << "Test failed at " << __FUNCSIG__ << "@" << __FILE__ << ":" << __LINE__ << " Iteration #" << Iteration << std::endl;\
	test_print_all(OutputStream, __VA_ARGS__);\
}

#define TEST_MIN_FLOAT 0
#define TEST_MAX_FLOAT 10000

bool test_eigen_equals(float a, float b, float tolerance)
{
	float err = std::abs(a - b);
	float tol = std::abs(tolerance * b);
	return err <= tol;
}

template <typename T, int N, int M>
bool test_eigen_equals(const matrix<T, N, M> & m, const Eigen::Matrix<T, N, M> & e, T tolerance)
{
	for (int j = 0; j < M; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			if (!test_eigen_equals(m(i, j), e(i, j), tolerance))
			{
				return false;
			}
		}
	}
	return true;
}

bool test_eigen_equals(const quaternion & q, const Eigen::Quaternionf & e, float tolerance)
{
	return (test_eigen_equals(q.w, e.w(), tolerance) &&
		test_eigen_equals(q.x, e.x(), tolerance) &&
		test_eigen_equals(q.y, e.y(), tolerance) &&
		test_eigen_equals(q.z, e.z(), tolerance));
}

template <typename T, int N, int M>
void test_eigen_load(const matrix<T, N, M> & m, Eigen::Matrix<T, N, M> & e)
{
	for (int j = 0; j < M; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			e(i, j) = m(i, j);
		}
	}
}

void test_eigen_load(const quaternion & q, Eigen::Quaternionf & e)
{
	e.w() = q.w;
	e.x() = q.x;
	e.y() = q.y;
	e.z() = q.z;
}

template <typename T, int N, int M>
bool test_eigen_add(
	const matrix<T, N, M> & a,
	const matrix<T, N, M> & b,
	matrix<T, N, M> & r,
	Eigen::Matrix<T, N, M> & e,
	T tolerance)
{
	Eigen::Matrix<T, N, M> e1, e2;
	test_eigen_load(a, e1);
	test_eigen_load(b, e2);
	r = a + b;
	e = e1 + e2;
	return test_eigen_equals(r, e, tolerance);
}

template <typename T, int N, int M>
bool test_eigen_sub(
	const matrix<T, N, M> & a,
	const matrix<T, N, M> & b,
	matrix<T, N, M> & r,
	Eigen::Matrix<T, N, M> & e,
	T tolerance)
{
	Eigen::Matrix<T, N, M> e1, e2;
	test_eigen_load(a, e1);
	test_eigen_load(b, e2);
	r = a - b;
	e = e1 - e2;
	return test_eigen_equals(r, e, tolerance);
}

template <typename T, int N, int M, int K>
bool test_eigen_multiply(
	const matrix<T, N, K> & a,
	const matrix<T, K, M> & b,
	matrix<T, N, M> & r,
	Eigen::Matrix<T, N, M> & e,
	T tolerance)
{
	Eigen::Matrix<T, N, K> e1;
	Eigen::Matrix<T, K, M> e2;
	test_eigen_load<T, N, K>(a, e1);
	test_eigen_load<T, K, M>(b, e2);
	r = a * b;
	e = e1 * e2;
	return test_eigen_equals<T, N, M>(r, e, tolerance);
}

template <typename T, int N, int M>
bool test_eigen_dot(
	const matrix<T, N, M> & a,
	const matrix<T, N, M> & b,
	float & r,
	float & e,
	T tolerance)
{
	Eigen::Matrix<T, N, M> e1, e2;
	test_eigen_load(a, e1);
	test_eigen_load(b, e2);
	r = dot(a, b);
	e = e1.dot(e2);
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_quaternion_mul(
	const quaternion & a,
	const quaternion & b,
	quaternion & r,
	Eigen::Quaternionf & e,
	float tolerance)
{
	Eigen::Quaternionf e1, e2;
	test_eigen_load(a, e1);
	test_eigen_load(b, e2);
	r = b * a;
	e = e2 * e1;
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_quaternion_transform(
	const quaternion & q,
	const float3 & x,
	float3 & r,
	Eigen::Matrix<float, 1, 3> & e,
	float tolerance)
{
	Eigen::Quaternionf e1;
	Eigen::Matrix<float, 1, 3> ex;
	test_eigen_load(q, e1);
	test_eigen_load(x, ex);
	r = transform(x, q);
	e = (Eigen::Matrix3f(e1) * ex.transpose()).transpose();
	//e = e1._transformVector(ex.transpose()).transpose();
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_cross_simd(
	const vec128_f32 & a,
	const vec128_f32 & b,
	vec128_f32 & r,
	Eigen::Matrix<float, 1, 3> & e,
	float tolerance)
{
	Eigen::Matrix<float, 1, 3> e1, e2;
	test_eigen_load(a.v3f, e1);
	test_eigen_load(b.v3f, e2);
	r = vec128_cross(a, b);
	e = e1.cross(e2);
	return test_eigen_equals(r.v3f, e, tolerance);
}

bool test_eigen_mul4_simd(
	const mat128_f32 & a,
	const mat128_f32 & b,
	mat128_f32 & r,
	Eigen::Matrix<float, 4, 4> & e,
	float tolerance)
{
	Eigen::Matrix<float, 4, 4> e1, e2;
	test_eigen_load(a.m, e1);
	test_eigen_load(b.m, e2);
	r = mat128_mul4(a, b);
	e = e1 * e2;
	return test_eigen_equals(r.m, e, tolerance);
}

bool test_eigen_inv4_simd(
	const mat128_f32 & a,
	mat128_f32 & r,
	Eigen::Matrix<float, 4, 4> & e,
	float tolerance)
{
	Eigen::Matrix<float, 4, 4> e1;
	test_eigen_load(a.m, e1);
	r = mat128_inv4(a, nullptr);
	e = e1.inverse();
	return test_eigen_equals(r.m, e, tolerance);
}

bool test_eigen_det4_simd(
	const mat128_f32 & a,
	float & r,
	float & e,
	float tolerance)
{
	Eigen::Matrix<float, 4, 4> e1;
	test_eigen_load(a.m, e1);
	r = vec128_get_x(mat128_det4(a));
	e = e1.determinant();
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_dot2_simd(
	const vec128_f32 & a,
	const vec128_f32 & b,
	float & r,
	float & e,
	float tolerance)
{
	Eigen::Matrix<float, 1, 2> e1, e2;
	test_eigen_load(a.v2f, e1);
	test_eigen_load(b.v2f, e2);
	r = ((vec128_f32)vec128_dp2(a, b)).f32[0];
	e = e1.dot(e2);
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_dot3_simd(
	const vec128_f32 & a,
	const vec128_f32 & b,
	float & r,
	float & e,
	float tolerance)
{
	Eigen::Matrix<float, 1, 3> e1, e2;
	test_eigen_load(a.v3f, e1);
	test_eigen_load(b.v3f, e2);
	r = ((vec128_f32)vec128_dp3(a, b)).f32[0];
	e = e1.dot(e2);
	return test_eigen_equals(r, e, tolerance);
}

bool test_eigen_dot4_simd(
	const vec128_f32 & a,
	const vec128_f32 & b,
	float & r,
	float & e,
	float tolerance)
{
	Eigen::Matrix<float, 1, 4> e1, e2;
	test_eigen_load(a.v4f, e1);
	test_eigen_load(b.v4f, e2);
	r = ((vec128_f32) vec128_dp4(a, b)).f32[0];
	e = e1.dot(e2);
	return test_eigen_equals(r, e, tolerance);
}

template <typename T, int N, int M>
bool test_eigen_length(
	const matrix<T, N, M> & a,
	float & r,
	float & e,
	T tolerance)
{
	Eigen::Matrix<T, N, M> e1;
	test_eigen_load(a, e1);
	r = length(a);
	e = e1.norm();
	return test_eigen_equals(r, e, tolerance);
}

template <typename Matrix, typename Distribution, typename Generator>
void test_load_random_matrix(Matrix & m, const Distribution & d, Generator & g)
{
	for (int i = 0; i < Matrix::size::value; ++i)
	{
		m[i] = d(g);
	}
}

template <typename Distribution, typename Generator>
void test_load_random_quaternion(quaternion & q, const Distribution & d, Generator & g)
{
	q.w = d(g);
	q.x = d(g);
	q.y = d(g);
	q.z = d(g);
}

template <typename T, int N, int M>
bool test_batch_eigen_add(int iterations, T tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<T> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		matrix<T, N, M> a;
		matrix<T, N, M> b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		matrix<T, N, M> r;
		Eigen::Matrix<T, N, M> e;
		
		if (!test_eigen_add(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

template <typename T, int N, int M>
bool test_batch_eigen_sub(int iterations, T tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<T> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		matrix<T, N, M> a;
		matrix<T, N, M> b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		matrix<T, N, M> r;
		Eigen::Matrix<T, N, M> e;

		if (!test_eigen_sub(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

template <typename T, int N, int K, int M>
bool test_batch_eigen_multiply(int iterations, T tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<T> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{
		matrix<T, N, K> a;
		matrix<T, K, M> b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		matrix<T, N, M> r;
		Eigen::Matrix<T, N, M> e;

		if (!test_eigen_multiply(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

template <typename T, int N, int M>
bool test_batch_eigen_dot(int iterations, T tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<T> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		matrix<T, N, M> a;
		matrix<T, N, M> b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		float r;
		float e;

		if (!test_eigen_dot(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_quaternion_mul(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		quaternion a;
		quaternion b;

		test_load_random_quaternion(a, dist, generator);
		test_load_random_quaternion(b, dist, generator);

		quaternion r;
		Eigen::Quaternionf e;

		if (!test_eigen_quaternion_mul(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_quaternion_transform(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		quaternion q;
		float3 x;

		test_load_random_quaternion(q, dist, generator);
		test_load_random_matrix(x, dist, generator);

		q = normalize(q);

		float3 r;
		Eigen::Matrix<float, 1, 3> e;

		if (!test_eigen_quaternion_transform(q, x, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", q, x, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", q, x, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_dot2_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4 a;
		float4 b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		float r;
		float e;

		if (!test_eigen_dot2_simd(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_dot3_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4 a;
		float4 b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		float r;
		float e;

		if (!test_eigen_dot3_simd(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_dot4_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4 a;
		float4 b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		float r;
		float e;

		if (!test_eigen_dot4_simd(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_cross_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4 a;
		float4 b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		vec128_f32 r;
		Eigen::Matrix<float, 1, 3> e;

		if (!test_eigen_cross_simd(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_mul4_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4x4 a;
		float4x4 b;

		test_load_random_matrix(a, dist, generator);
		test_load_random_matrix(b, dist, generator);

		mat128_f32 r;
		Eigen::Matrix<float, 4, 4> e;

		if (!test_eigen_mul4_simd(a, b, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, b, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_inv4_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4x4 a;

		test_load_random_matrix(a, dist, generator);

		mat128_f32 r;
		Eigen::Matrix<float, 4, 4> e;

		if (!test_eigen_inv4_simd(a, r, e, tolerance))
		{
			float4x4 I = a * r.m;
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, "Result:", r, "Eigen result:", e, "Identity:", I);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, "Result:", r, "Eigen result:", e, "Identity:", I);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

bool test_batch_eigen_det4_simd(int iterations, float tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<float> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		float4x4 a;

		test_load_random_matrix(a, dist, generator);

		float r;
		float e;

		if (!test_eigen_det4_simd(a, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

template <typename T, int N, int M>
bool test_batch_eigen_length(int iterations, T tolerance)
{

	std::random_device device;
	std::mt19937 generator(device());
	std::uniform_real_distribution<T> dist(TEST_MIN_FLOAT, TEST_MAX_FLOAT);

	for (int i = 0; i < iterations; i++)
	{

		matrix<T, N, M> a;

		test_load_random_matrix(a, dist, generator);

		float r;
		float e;

		if (!test_eigen_length(a, r, e, tolerance))
		{
			TEST_FAIL_LOG(std::cout, i, "Arguments:", a, "Result:", r, "Eigen result:", e);
			TEST_FAIL_LOG(g_log, i, "Arguments:", a, "Result:", r, "Eigen result:", e);
			return false;
		}

	}

	TEST_SUCCESS_LOG(g_log);
	TEST_SUCCESS_LOG(std::cout);

	return true;

}

int main(int argc, char * argv[])
{

	/*float3 a(1, 2, 3);
	float3 b(4, 5, 6);

	std::cout << dot(a, b) << std::endl;
	std::cout << dot(transpose(a), transpose(b)) << std::endl;
	std::cout << length(a) << std::endl;
	std::cout << length(transpose(a)) << std::endl;
	std::cout << length(b) << std::endl;
	std::cout << length(transpose(b)) << std::endl;
	std::cout << detail::is_matrix<float>::value << std::endl;
	std::cout << detail::is_matrix<Eigen::Matrix3f>::value << std::endl;
	std::cout << detail::is_matrix<float3x3>::value << std::endl;
	std::cout << detail::is_vector<float3>::value << std::endl;
	std::cout << detail::is_vector<float3x3>::value << std::endl;
	std::cout << detail::is_vector<float3col>::value << std::endl;*/
	
	/*float3x3 x = float3x3::identity();
	matrix<float, 3, 4> y = {};
	float3x3 t = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

	x = t;

	float3 v{ 1, 2, 3 };
	float4 v2(4, 5, 6, 7);

	std::cout << x * 3 << std::endl;
	std::cout << v << std::endl;
	std::cout << t << std::endl;
	std::cout << t * t << std::endl;
	std::cout << v * t << std::endl;
	std::cout << v * transpose(t) << std::endl;
	std::cout << t * transpose(v) << std::endl;
	std::cout << v2 / 3 << std::endl;*/

	const int   Iterations = 30;
	const float Tolerance  = 1e-05;
	const float Tolerance2 = 1e-03;

	g_log.open("math_test.log");

//#include "test_batch_eigen_add.inl"
//#include "test_batch_eigen_sub.inl"
//#include "test_batch_eigen_multiply.inl"
#include "test_batch_eigen_dot.inl"
#include "test_batch_eigen_length.inl"

	test_batch_eigen_quaternion_mul(Iterations, Tolerance);
	test_batch_eigen_quaternion_transform(Iterations, Tolerance);

	test_batch_eigen_dot2_simd(Iterations, Tolerance);
	test_batch_eigen_dot3_simd(Iterations, Tolerance);
	test_batch_eigen_dot4_simd(Iterations, Tolerance);

	test_batch_eigen_cross_simd(Iterations, Tolerance);

	test_batch_eigen_mul4_simd(Iterations, Tolerance);
	test_batch_eigen_inv4_simd(Iterations, Tolerance2);
	test_batch_eigen_det4_simd(Iterations, Tolerance2);

	/*vec128_f32 a(1, 2, 3, 4);
	vec128_f32 b(4, 5, 6, 7);

	vec128_f32 c = vec128_shuffle<FUSE_Y0, FUSE_X0, FUSE_Z1, FUSE_Y1>(b, a);
	vec128_f32 d = vec128_swizzle<FUSE_W, FUSE_Z, FUSE_Y, FUSE_X>(c);
	vec128_f32 e = vec128_splat<FUSE_Z>(b);

	vec128_f32 f = c + d;

	TEST_FAIL_LOG(g_log, 0, a.v4f, b.v4f, c.v4f, d.v4f, e.v4f, f.v4f);

	mat128_f32 m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
	mat128_f32 m1 = mat128_trans4(m0);

	TEST_FAIL_LOG(g_log, 1, m0, m1);*/

	/*float3 a(1, 2, 3);
	float3 b(-36, 5, -4);

	float3 c = a + b;
	float3 d = add(a, b);

	std::cout << to_string(a + b) << std::endl;
	std::cout << to_string(absolute(a + b)) << std::endl;

	std::cout << absolute(-25.324f) << std::endl;
	std::cout << absolute(-12.77f) << std::endl;
	std::cout << absolute(-230) << std::endl;

	float4x4 m(
		1, 2, 3, 4,
		5, 6, 7, 8,
		9, 10, 11, 12,
		13, 14, 15, 16);

	std::cout << m << std::endl;

	std::cout << m * m << std::endl;*/

	return 0;

}