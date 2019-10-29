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
add_subdirectory(librtk)

# in case OpenZen includes the install target, spdlog also
# needs to be forced to generate its install target. Normally
# it does not do that if it is in a sub-folder
if (ZEN_INSTALL)
    set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(spdlog)
