set(example_sources
    main.cpp
)

add_executable(OpenZenExample
    ${example_sources}
)

target_link_libraries(OpenZenExample
    PRIVATE
        OpenZen
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

add_custom_command(TARGET OpenZenExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt5::Bluetooth>
        $<TARGET_FILE_DIR:OpenZenExample>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt5::Core>
        $<TARGET_FILE_DIR:OpenZenExample>
)