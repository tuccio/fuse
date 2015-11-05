cmake_minimum_required ( VERSION 2.8 )

project ( renderer )

add_executable ( renderer WIN32
	renderer.cpp
	renderer_application.hpp renderer_application.cpp
	deferred_renderer.hpp deferred_renderer.cpp
	scene.hpp scene.cpp
	cbuffer_structs.hpp
	light.hpp
	renderable.hpp
)

target_link_libraries ( renderer fuse ${DirectX12_LIBRARY} )
target_link_libraries ( renderer fuse ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES} )
target_link_libraries ( renderer fuse ${ASSIMP_LIBRARY} )