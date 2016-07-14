cmake_minimum_required ( VERSION 2.8 )

project ( math )

file ( GLOB FUSE_MATH_SOURCE_FILES math/*.cpp )
file ( GLOB FUSE_MATH_INCLUDE_FILES include/fuse/math/*.hpp include/fuse/math/*.inl include/fuse/math.hpp )

file ( GLOB FUSE_GEOMETRY_SOURCE_FILES geometry/*.cpp )
file ( GLOB FUSE_GEOMETRY_INCLUDE_FILES include/fuse/geometry/*.hpp include/fuse/geometry/*.inl include/fuse/geometry.hpp )

source_group ( "math\\include" FILES ${FUSE_MATH_INCLUDE_FILES} )
source_group ( "math\\source" FILES ${FUSE_MATH_SOURCE_FILES} )

source_group ( "geometry\\include" FILES ${FUSE_GEOMETRY_INCLUDE_FILES} )
source_group ( "geometry\\source" FILES ${FUSE_GEOMETRY_SOURCE_FILES} )

add_library ( fusemath ${FUSE_MATH_INCLUDE_FILES} ${FUSE_MATH_SOURCE_FILES} ${FUSE_GEOMETRY_INCLUDE_FILES} ${FUSE_GEOMETRY_SOURCE_FILES} )