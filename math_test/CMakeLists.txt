cmake_minimum_required ( VERSION 2.8 )

project ( math_test )

file ( GLOB FUSE_MATH_TEST_SRC_FILES *.cpp *.hpp )

include_directories ( Eigen )

add_executable ( math_test ${FUSE_MATH_TEST_SRC_FILES} )
target_link_libraries ( math_test fusegraphics fusemath )