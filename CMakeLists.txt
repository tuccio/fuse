cmake_minimum_required ( VERSION 2.8 )

project ( fuse )

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )
set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )
set ( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib )

option ( FUSE_SSE2   "Enables SSE intruction set" ON )
option ( FUSE_ASSIMP "Enables Assimp support"     ON )

find_package ( Boost     REQUIRED )
find_package ( DirectX12 REQUIRED )
find_package ( DevIL )
find_package ( Assimp )
#find_package ( OpenMP )

include_directories ( ${DirectX12_INCLUDE_DIR} )
include_directories ( ${Boost_INCLUDE_DIRS} )
include_directories ( ${CMAKE_SOURCE_DIR}/include )
include_directories ( ${IL_INCLUDE_DIR} )

set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /fp:fast )

if (FUSE_SSE2)

	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /arch:SSE2 )

endif (FUSE_SSE2)

if ( ASSIMP_FOUND AND FUSE_ASSIMP )

	add_definitions ( -DFUSE_ASSIMP )
	include_directories ( ${ASSIMP_INCLUDE_DIR} )
	
endif ( ASSIMP_FOUND AND FUSE_ASSIMP )

add_library ( fuse
	include/fuse/singleton.hpp
	include/fuse/lockable.hpp
	include/fuse/logger.hpp logger.cpp
	include/fuse/resource.hpp resource.cpp
	include/fuse/resource_types.hpp
	include/fuse/resource_factory.hpp resource_factory.cpp
	include/fuse/resource_manager.hpp resource_manager.cpp
	include/fuse/image.hpp image.cpp
	include/fuse/image_manager.hpp
	include/fuse/material.hpp material.cpp
	include/fuse/material_manager.hpp
	include/fuse/assimp_loader.hpp assimp_loader.cpp
	include/fuse/mesh.hpp mesh.cpp
	include/fuse/mesh_manager.hpp
	include/fuse/model.hpp model.cpp
	include/fuse/application.hpp application.cpp
	include/fuse/directx_helper.hpp
	include/fuse/compile_shader.hpp compile_shader.cpp
	include/fuse/pipeline_state.hpp pipeline_state.cpp
	include/fuse/gpu_upload_manager.hpp gpu_upload_manager.cpp
	include/fuse/gpu_mesh.hpp gpu_mesh.cpp
	include/fuse/gpu_mesh_manager.hpp
	include/fuse/camera.hpp camera.cpp
	include/fuse/transform.hpp transform.cpp
	include/fuse/properties_macros.hpp
	include/fuse/math.hpp
	include/fuse/fps_camera_controller.hpp fps_camera_controller.cpp
	include/fuse/input.hpp input.cpp
	include/fuse/allocators.hpp
	include/fuse/debug_ostream.hpp
	include/fuse/gpu_command_queue.hpp gpu_command_queue.cpp
	include/fuse/gpu_ring_buffer.hpp gpu_ring_buffer.cpp
)

add_subdirectory ( sample )
add_subdirectory ( renderer )