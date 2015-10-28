cmake_minimum_required ( VERSION 2.8 )

project ( sample )

add_executable ( sample WIN32 sample.cpp )
target_link_libraries ( sample fuse ${DirectX12_LIBRARY} )