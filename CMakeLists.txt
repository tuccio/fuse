cmake_minimum_required ( VERSION 2.8 )

project ( fuse )

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )
set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )
set ( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib )

option ( FUSE_SSE2         "Enables SSE intruction set"                                 ON )
option ( FUSE_ASSIMP       "Enables Assimp support if available"                        ON )
option ( FUSE_SHADER_DEBUG "Enables shader debugging (disables compiler optimizations)" OFF )
option ( FUSE_WXWIDGETS    "Enables wxWidgets support if available"                     ON )
option ( FUSE_UNICODE      "Enables Unicode charset (might be enabled automatically if some components require it)." ON )

find_package ( Boost     REQUIRED )
find_package ( DirectX12 REQUIRED )
find_package ( DevIL     REQUIRED )
find_package ( Assimp )
find_package ( wxWidgets COMPONENTS core base aui )
#find_package ( OpenMP )

include_directories ( ${DirectX12_INCLUDE_DIR} )
include_directories ( ${Boost_INCLUDE_DIRS} )
include_directories ( ${CMAKE_SOURCE_DIR}/include )
include_directories ( ${IL_INCLUDE_DIR} )

set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /fp:fast )

file ( GLOB FUSE_CORE_SOURCE_FILES *.cpp )
file ( GLOB FUSE_CORE_INCLUDE_FILES include/fuse/*.hpp include/fuse/*.inl )

file ( GLOB FUSE_MATH_SOURCE_FILES math/*.cpp )
file ( GLOB FUSE_MATH_INCLUDE_FILES include/fuse/math/*.hpp include/fuse/math/*.inl )

file ( GLOB FUSE_GEOMETRY_SOURCE_FILES geometry/*.cpp )
file ( GLOB FUSE_GEOMETRY_INCLUDE_FILES include/fuse/geometry/*.hpp include/fuse/geometry/*.inl )

source_group ( "core\\include" FILES ${FUSE_CORE_INCLUDE_FILES} )
source_group ( "core\\source" FILES ${FUSE_CORE_SOURCE_FILES} )

source_group ( "math\\include" FILES ${FUSE_MATH_INCLUDE_FILES} )
source_group ( "math\\source" FILES ${FUSE_MATH_SOURCE_FILES} )

source_group ( "geometry\\include" FILES ${FUSE_GEOMETRY_INCLUDE_FILES} )
source_group ( "geometry\\source" FILES ${FUSE_GEOMETRY_SOURCE_FILES} )

set ( FUSE_SRC_FILES
	${FUSE_CORE_SOURCE_FILES} ${FUSE_CORE_INCLUDE_FILES}
	${FUSE_MATH_SOURCE_FILES} ${FUSE_MATH_INCLUDE_FILES}
	${FUSE_GEOMETRY_SOURCE_FILES} ${FUSE_GEOMETRY_INCLUDE_FILES}
	)

set ( libRocket_INCLUDE_DIR libRocket/include )

set (
	libRocket_LIBRARIES
	optimized ${CMAKE_SOURCE_DIR}/libRocket/lib/Release/RocketCore.lib
	optimized ${CMAKE_SOURCE_DIR}/libRocket/lib/Release/RocketControls.lib
	optimized ${CMAKE_SOURCE_DIR}/libRocket/lib/Release/RocketDebugger.lib
	debug ${CMAKE_SOURCE_DIR}/libRocket/lib/Debug/RocketCore.lib
	debug ${CMAKE_SOURCE_DIR}/libRocket/lib/Debug/RocketControls.lib
	debug ${CMAKE_SOURCE_DIR}/libRocket/lib/Debug/RocketDebugger.lib
)

set ( freetype_LIBRARY ${CMAKE_SOURCE_DIR}/freetype/lib/freetype240.lib )

add_definitions ( -DROCKET_STATIC_LIB )
add_definitions ( -DNOMINMAX )

if ( FUSE_SSE2 )
	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /arch:SSE2 )
endif ( FUSE_SSE2 )

if ( FUSE_SHADER_DEBUG )
	add_definitions ( "-DFUSE_COMPILE_SHADER_GLOBAL_OPTIONS=(D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_ENABLE_STRICTNESS)" )
endif ( FUSE_SHADER_DEBUG )

if ( ASSIMP_FOUND AND FUSE_ASSIMP )
	add_definitions ( -DFUSE_ASSIMP )
	include_directories ( ${ASSIMP_INCLUDE_DIR} )
endif ( ASSIMP_FOUND AND FUSE_ASSIMP )

include_directories ( ${libRocket_INCLUDE_DIR} )

if ( wxWidgets_FOUND AND FUSE_WXWIDGETS )
	include ( ${wxWidgets_USE_FILE} )	
	add_definitions ( -DFUSE_WXWIDGETS )
endif ( wxWidgets_FOUND AND FUSE_WXWIDGETS )

if ( FUSE_UNICODE OR FUSE_WXWIDGETS )
	add_definitions ( -DFUSE_UNICODE -DUNICODE -D_UNICODE )
endif ( FUSE_UNICODE OR FUSE_WXWIDGETS )

add_library ( fuse ${FUSE_SRC_FILES} )

if ( wxWidgets_FOUND AND FUSE_WXWIDGETS )
	target_link_libraries ( fuse ${wxWidgets_LIBRARIES} )
endif ( wxWidgets_FOUND AND FUSE_WXWIDGETS )

add_subdirectory ( renderer )
add_subdirectory ( math_test )