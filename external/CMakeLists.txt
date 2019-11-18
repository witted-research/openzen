# EXCLUDE_FROM_ALL is needed for the header-only libraries
# so their headers and cmake files are not copied to the
# output folder when "make install" is used

add_subdirectory(expected-lite EXCLUDE_FROM_ALL)
add_subdirectory(ftdi)
add_subdirectory(gsl EXCLUDE_FROM_ALL)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(googletest EXCLUDE_FROM_ALL)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(nlohmann-json EXCLUDE_FROM_ALL)

add_subdirectory(pcanbasic)
add_subdirectory(siusb)
set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)
add_subdirectory(cereal)

# in case OpenZen includes the install target, spdlog also
# needs to be forced to generate its install target. Normally
# it does not do that if it is in a sub-folder
if (ZEN_INSTALL)
    set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(spdlog)

# expose the Asio headers as library
add_library(openzen_asio INTERFACE)
target_include_directories(openzen_asio INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include
    )

if (ZEN_NETWORK)
    set(ENABLE_PRECOMPILED OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(libzmq)

    # expose the cppzmq header as library because the cmake file coming
    # with the library uses CMake's find to look up the system-installed
    # zero mq
    add_library(cppzmq INTERFACE)
    target_include_directories(cppzmq INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/cppzmq
        )
endif()
