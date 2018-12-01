# Determine platform (32/64)
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	set(X64 ON)
else()
	set(X86 ON)
endif()

add_library(ftdi MODULE IMPORTED GLOBAL)

set_target_properties(ftdi PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
	if (X64)
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win64/ftd2xx.dll)
	else()
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win32/ftd2xx.dll)
	endif()

	set_target_properties(ftdi PROPERTIES
		IMPORTED_LOCATION ${SHARED_LIB}
	)
endif()
