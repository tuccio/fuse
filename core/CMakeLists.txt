cmake_minimum_required ( VERSION 2.8 )

project ( core )

file ( GLOB FUSE_CORE_SOURCE_FILES *.cpp )
file ( GLOB FUSE_CORE_INCLUDE_FILES include/fuse/*.hpp include/fuse/*.inl include/fuse/core/*.hpp include/fuse/core/*.inl )

add_library ( fusecore ${FUSE_CORE_SOURCE_FILES} ${FUSE_CORE_INCLUDE_FILES} )