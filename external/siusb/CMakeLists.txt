# Determine platform (32/64)
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	set(X64 ON)
else()
	set(X86 ON)
endif()

if(WIN32)
	add_library(siusb MODULE IMPORTED GLOBAL)

	set_target_properties(siusb PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
	)

	if (X64)
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win64/siUSBXp.dll)
	else()
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win32/siUSBXp.dll)
	endif()

	set_target_properties(siusb PROPERTIES
		IMPORTED_LOCATION ${SHARED_LIB}
	)
endif()
