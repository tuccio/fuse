cmake_minimum_required ( VERSION 2.8 )

project ( renderer )

option ( FUSE_EDITOR_GUI "Compiles with editor GUI (Requires AntTweakBar)" OFF )

add_executable ( 
	renderer WIN32
	renderer.cpp
	renderer_application.hpp renderer_application.cpp
	deferred_renderer.hpp deferred_renderer.cpp
	scene.hpp scene.cpp
	cbuffer_structs.hpp
	light.hpp
	renderable.hpp
	tonemapper.hpp tonemapper.cpp
	shadow_mapper.hpp shadow_mapper.cpp
	editor_gui.hpp editor_gui.cpp
	render_variables.hpp render_variables.cpp
	blur.hpp blur.cpp
)

target_link_libraries ( renderer fuse ${DirectX12_LIBRARY} )
target_link_libraries ( renderer ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES} )
target_link_libraries ( renderer ${ASSIMP_LIBRARY} )

if ( FUSE_EDITOR_GUI )
	add_definitions ( -DFUSE_USE_EDITOR_GUI )
	include_directories ( ${libRocket_INCLUDE_DIR} )
	target_link_libraries ( renderer ${libRocket_LIBRARIES} ${freetype_LIBRARY} )
endif ( FUSE_EDITOR_GUI )