cmake_minimum_required ( VERSION 2.8 )

project ( fuse )

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )
set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin )
set ( LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib )

option ( FUSE_SSE2         "Enables SSE intruction set"                                 ON )
option ( FUSE_ASSIMP       "Enables Assimp support"                                     ON )
option ( FUSE_SHADER_DEBUG "Enables shader debugging (disables compiler optimizations)" OFF )

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

if (FUSE_SSE2)

	set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} /arch:SSE2 )

endif (FUSE_SSE2)

if (FUSE_SHADER_DEBUG)
	add_definitions ( "-DFUSE_COMPILE_SHADER_GLOBAL_OPTIONS=(D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_ENABLE_STRICTNESS)" )
endif (FUSE_SHADER_DEBUG)

if ( ASSIMP_FOUND AND FUSE_ASSIMP )

	add_definitions ( -DFUSE_ASSIMP )
	include_directories ( ${ASSIMP_INCLUDE_DIR} )
	
endif ( ASSIMP_FOUND AND FUSE_ASSIMP )

include_directories ( ${libRocket_INCLUDE_DIR} )

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
	include/fuse/color.hpp color.cpp
	include/fuse/material.hpp material.cpp
	include/fuse/material_manager.hpp
	include/fuse/descriptor_heap.hpp descriptor_heap.cpp
	include/fuse/pool_manager.hpp pool_manager.cpp
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
	include/fuse/gpu_render_context.hpp gpu_render_context.cpp
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
	include/fuse/bitmap_font.hpp bitmap_font.cpp
	include/fuse/bitmap_font_manager.hpp
	include/fuse/text_renderer.hpp text_renderer.cpp
	include/fuse/texture.hpp texture.cpp
	include/fuse/render_resource.hpp render_resource.cpp
	include/fuse/render_resource_manager.hpp render_resource_manager.cpp
	include/fuse/texture_manager.hpp
	include/fuse/gpu_graphics_command_list.hpp gpu_graphics_command_list.cpp
	include/fuse/gpu_global_resource_state.hpp
	include/fuse/mipmap_generator.hpp mipmap_generator.cpp
	include/fuse/rocket_interface.hpp rocket_interface.cpp
	include/fuse/rocket_input.hpp rocket_input.cpp
	include/fuse/leading_zeros.hpp
	include/fuse/morton.hpp
	include/fuse/geometry.hpp
	include/fuse/geometry/sphere.hpp sphere.cpp
	include/fuse/geometry/aabb.hpp aabb.cpp
	include/fuse/geometry/frustum.hpp frustum.cpp
	include/fuse/geometry/plane.hpp plane.cpp
	include/fuse/geometry/ray.hpp ray.cpp
	include/fuse/geometry/bounding_volumes.hpp
	include/fuse/geometry/intersection.hpp include/fuse/geometry/intersection.inl
	include/fuse/geometry/transform_affine.hpp
	include/fuse/geometry/loose_octree.hpp include/fuse/geometry/loose_octree.inl
)

add_subdirectory ( renderer )