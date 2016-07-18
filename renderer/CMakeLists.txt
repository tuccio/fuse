cmake_minimum_required ( VERSION 2.8 )

project ( renderer )

option ( FUSE_EDITOR_GUI "Compiles with editor GUI" OFF )

file ( GLOB FUSE_RENDERER_SRC_FILES *.cpp *.hpp *.inl )

add_executable ( renderer WIN32 ${FUSE_RENDERER_SRC_FILES} )

target_link_libraries ( renderer fusecore fusemath fusegraphics )
target_link_libraries ( renderer ${DirectX12_LIBRARY} )
target_link_libraries ( renderer ${IL_LIBRARIES} ${ILU_LIBRARIES} ${ILUT_LIBRARIES} )
target_link_libraries ( renderer ${ASSIMP_LIBRARY} )

if ( FUSE_EDITOR_GUI )
	add_definitions ( -DFUSE_USE_EDITOR_GUI )
	include_directories ( ${libRocket_INCLUDE_DIR} )
	target_link_libraries ( renderer ${libRocket_LIBRARIES} ${freetype_LIBRARY} )
endif ( FUSE_EDITOR_GUI )


if ( wxWidgets_FOUND AND FUSE_WXWIDGETS )
	target_link_libraries ( renderer ${wxWidgets_LIBRARIES} )
endif ( wxWidgets_FOUND AND FUSE_WXWIDGETS )