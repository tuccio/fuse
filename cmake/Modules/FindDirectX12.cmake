if (WIN32)

	if (CMAKE_CL_64)
		set ( DirectX12_ARCH "x64" )
	else()
		set ( DirectX12_ARCH "x86" )
	endif()

	set (ENV_PROGRAMFILES $ENV{ProgramFiles})
	
	set (
		DirectX12_INC_PATH_SUFFIXES
		um
		shared
	)
	
	set (
		DirectX12_LIB_PATH_SUFFIXES
		${DirectX12_ARCH}
		um
	)
	set (
		WindowsSDK_ROOT
		"${ENV_PROGRAMFILES}/Windows Kits"
	)
	
	find_path ( DirectX12_D3D12_INCLUDE_DIR NAMES d3d12.h HINTS "${WindowsSDK_ROOT}/10/Include/*" PATH_SUFFIXES ${DirectX12_INC_PATH_SUFFIXES} )
	find_path ( DirectX12_DXGI_INCLUDE_DIR NAMES dxgi1_4.h HINTS "${WindowsSDK_ROOT}/10/Include/*" PATH_SUFFIXES ${DirectX12_INC_PATH_SUFFIXES} )
	
	find_library ( DirectX12_D3D12_LIBRARY NAMES d3d12 HINTS "${WindowsSDK_ROOT}/10/Lib/*/*" PATH_SUFFIXES ${DirectX12_LIB_PATH_SUFFIXES} )
	find_library ( DirectX12_D3DCOMPILER_LIBRARY NAMES d3dcompiler HINTS "${WindowsSDK_ROOT}/10/Lib/*/*" PATH_SUFFIXES ${DirectX12_LIB_PATH_SUFFIXES} )
	find_library ( DirectX12_DXGI_LIBRARY NAMES dxgi HINTS "${WindowsSDK_ROOT}/10/Lib/*/*" PATH_SUFFIXES ${DirectX12_LIB_PATH_SUFFIXES} )
	
	if (DirectX12_D3D12_INCLUDE_DIR AND DirectX12_DXGI_INCLUDE_DIR)
		set ( DirectX12_FOUND TRUE)
	else ()
		set ( DirectX12_FOUND FALSE )
	endif ()
	
	if (DirectX12_FOUND)
	
		set ( DirectX12_INCLUDE_DIR ${DirectX12_D3D12_INCLUDE_DIR} ${DirectX12_DXGI_INCLUDE_DIR} )
		set ( DirectX12_LIBRARY ${DirectX12_D3D12_LIBRARY} ${DirectX12_D3DCOMPILER_LIBRARY} ${DirectX12_DXGI_LIBRARY} )
		
	elseif (DirectX12_FIND_REQUIRED)
	
		message ( FATAL_ERROR "Could not find DirectX12" )
		
	endif()

endif ()