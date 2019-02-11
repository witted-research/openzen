add_subdirectory(expected-lite)
add_subdirectory(ftdi)
add_subdirectory(gsl)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(nlohmann-json)

add_subdirectory(pcanbasic)
add_subdirectory(siusb)
