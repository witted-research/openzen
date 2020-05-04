set(example_sources
    main.cpp
)

add_executable(OpenZenExample
    ${example_sources}
)

include(FindThreads)

target_compile_features(OpenZenExample
    PRIVATE
        cxx_std_14
)

target_link_libraries(OpenZenExample
    PRIVATE
        OpenZen
        GSL
        Threads::Threads
        spdlog
)

if(ZEN_USE_STATIC_LIBS)
    target_compile_definitions(OpenZenExample
        PRIVATE
            ZEN_API_STATIC
    )
else()
    add_custom_command(TARGET OpenZenExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:OpenZen>
            $<TARGET_FILE_DIR:OpenZenExample>
    )
endif()

# only copy Qt libraries if Bluetooth BLE support is enabled
if(ZEN_BLUETOOTH_BLE)
    add_custom_command(TARGET OpenZenExample POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt5::Bluetooth>
            $<TARGET_FILE_DIR:OpenZenExample>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
           $<TARGET_FILE:Qt5::Core>
            $<TARGET_FILE_DIR:OpenZenExample>
    )
endif()

if (MSVC)
    target_compile_options(OpenZenExample
        PRIVATE
            /Zc:__cplusplus
    )
endif()

# Disabled for now, as it results on compiler errors with Visual Studio static
# runtime linking
if (!MSVC)
    add_executable(OpenZenExampleCAPI
        ExampleCAPI.c)

    target_link_libraries(OpenZenExampleCAPI
        OpenZen)
endif()

add_executable(OpenZenExampleCpp
    ExampleCpp.cpp)

target_link_libraries(OpenZenExampleCpp
    OpenZen)

target_compile_features(OpenZenExampleCpp
    PRIVATE
        cxx_std_11
)
