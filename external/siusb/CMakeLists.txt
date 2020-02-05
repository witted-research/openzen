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
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win64/SiUSBXp.dll)
	else()
		set(SHARED_LIB ${CMAKE_CURRENT_SOURCE_DIR}/win32/SiUSBXp.dll)
	endif()

	set_target_properties(siusb PROPERTIES
		IMPORTED_LOCATION ${SHARED_LIB}
	)

    # copy the required DLL to the output folder, so the examples can run
    configure_file(${SHARED_LIB} ${CMAKE_BINARY_DIR} COPYONLY)
    # this cmake file might be the first to access the examples folder on a
    # clean-slate build, so make sure it exists.
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    configure_file(${SHARED_LIB} ${CMAKE_BINARY_DIR}/examples COPYONLY)
endif()
