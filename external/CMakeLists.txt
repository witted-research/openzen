# EXCLUDE_FROM_ALL is needed for the header-only libraries
# so their headers and cmake files are not copied to the
# output folder when "make install" is used

if (ZEN_USE_STATIC_LIBS)
    # install spdlog in case of static build
    # because CMake will otherwise complain about
    # missing export target
    set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
    set(PYBIND11_INSTALL ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(expected-lite EXCLUDE_FROM_ALL)
add_subdirectory(gsl EXCLUDE_FROM_ALL)

if (ZEN_TESTS)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif()

# download binary driver libraries for windows if
# we compile for Win32 and we are allowed by the build
# options todo so
if (WIN32 AND ZEN_USE_BINARY_LIBRARIES)
    message("Binary libraries will be downloaded")

    # FetchContent is supported since CMake Version 3.11
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
        binary_libraries
        GIT_REPOSITORY https://bitbucket.org/lpresearch/openzenbinarylibraries.git
        GIT_TAG        v1.0.0
    )

    FetchContent_GetProperties(binary_libraries)

    if (NOT binary_libraries_POPULATED)
      FetchContent_Populate(binary_libraries)
      add_subdirectory(${binary_libraries_SOURCE_DIR} ${binary_libraries_BINARY_DIR})
    endif()
else()
    message("Binary libraries will not be downloaded")
endif()

if (ZEN_PYTHON)
    add_subdirectory(pybind11)
endif()

# tells cereal to not build any tests
set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)
# we don't want cereals install targets, so use
# EXCLUDE_FROM_ALL on this folder

# currently, the cereal external gives an cmake warning
# because project() call does not set version. There is a PR in the pipeline
# which will fix this in the future
# https://github.com/USCiLab/cereal/pull/659
add_subdirectory(cereal EXCLUDE_FROM_ALL)

add_subdirectory(spdlog EXCLUDE_FROM_ALL)
set_property(TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cppzmq>
        $<INSTALL_INTERFACE:include/cppzmq>
        )
endif()
