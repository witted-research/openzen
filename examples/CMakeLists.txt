set(example_sources
    main.cpp
)

add_executable(OpenZenExample
    ${example_sources}
)

include(FindThreads)

target_compile_features(OpenZenExample
    PRIVATE
        cxx_std_17
)

target_link_libraries(OpenZenExample
    PRIVATE
        OpenZen
        GSL
        Threads::Threads
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

# only copy Qt libraries if bluetooth support is enabled
if(ZEN_BLUETOOTH)
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