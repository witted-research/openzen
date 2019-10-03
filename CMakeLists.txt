cmake_minimum_required(VERSION 3.10)
project(OpenZen VERSION 0.1.0 LANGUAGES CXX;C)

add_subdirectory(external)

# Check if OpenZen is being used directly or via add_subdirectory
set(OPENZEN_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(OPENZEN_MASTER_PROJECT ON)
endif()

option(ZEN_USE_STATIC_LIBS "Whether to compile OpenZen as a static library" OFF)
option(ZEN_STATIC_LINK_LIBCXX "Option to statically link libstdc++ to be portable to older systems (Linux only)" OFF)
option(ZEN_BLUETOOTH "Compile OpenZen with bluetooth support" ON)
option(ZEN_CSHARP "Compile C# bindings for OpenZen" ON)
option(ZEN_INSTALL "Generate the install target." ${OPENZEN_MASTER_PROJECT})


# Enable this line and set it to the correct Qt installation path if you want to compile
# with bluetooth support on Windows
#set(CMAKE_PREFIX_PATH "C://Qt//5.12.3//msvc2017_64//")

#----------------------------------------------------------------
# Packages
#----------------------------------------------------------------
# Find includes in corresponding build include_directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# contains additional C/C++ precompiler defines
# which might be set depending on the selected compile options
set(zen_optional_compile_definitions_private)

if (ZEN_BLUETOOTH)
    # Instruct CMake to run moc automatically when needed
    set(CMAKE_AUTOMOC ON)
    find_package(Qt5Bluetooth REQUIRED)
endif()

set(zen_includes
    include/OpenZen.h
    include/OpenZenCAPI.h
    include/ZenProtocol.h
    include/ZenTypes.h
)

set(zen_sources
    src/ImuHelpers.h
    src/InternalTypes.h
    src/ISensorProperties.cpp
    src/ISensorProperties.h
    src/OpenZen.cpp
    src/Sensor.cpp
    src/Sensor.h
    src/SensorClient.cpp
    src/SensorClient.h
    src/SensorConfig.h
    src/SensorComponent.h
    src/SensorManager.cpp
    src/SensorManager.h
    src/SensorProperties.cpp
    src/SensorProperties.h

    src/LpMatrix.c
    src/LpMatrix.h
)

if(ZEN_CSHARP)
    list (APPEND zen_sources
        ## run
        # swig.exe -csharp -small -c++ -debug-typedef -DSWIGWORDSIZE64 -o OpenZenCSharp/OpenZen_wrap_csharp.cxx -outdir OpenZenCSharp OpenZen.i
        # in the bindings folder to generate
        bindings/OpenZenCSharp/OpenZen_wrap_csharp.cxx
    )
endif()

set(zen_optional_libs)

set(communication_sources
    src/communication/ConnectionNegotiator.cpp
    src/communication/ConnectionNegotiator.h
    src/communication/Modbus.cpp
    src/communication/Modbus.h
    src/communication/ModbusCommunicator.cpp
    src/communication/ModbusCommunicator.h
    src/communication/SyncedModbusCommunicator.cpp
    src/communication/SyncedModbusCommunicator.h
)

set(components_sources
    src/components/ComponentFactoryManager.cpp
    src/components/ComponentFactoryManager.h
    src/components/IComponentFactory.h
    src/components/ImuComponent.cpp
    src/components/ImuComponent.h
    src/components/ImuIg1Component.cpp
    src/components/ImuIg1Component.h
    src/components/GnssComponent.cpp
    src/components/GnssComponent.h
)

set(components_factories_sources
    src/components/factories/ImuComponentFactory.cpp
    src/components/factories/ImuComponentFactory.h
    src/components/factories/GnssComponentFactory.cpp
    src/components/factories/GnssComponentFactory.h
)

set(io_sources
    src/io/IIoInterface.h
    src/io/IIoSystem.h
    src/io/IoManager.cpp
    src/io/IoManager.h
)

set(io_can_sources
    src/io/can/CanManager.cpp
    src/io/can/CanManager.h
    src/io/can/ICanChannel.h
)

set(io_interfaces_sources
    src/io/interfaces/CanInterface.cpp
    src/io/interfaces/CanInterface.h
    src/io/interfaces/FtdiUsbInterface.cpp
    src/io/interfaces/FtdiUsbInterface.h
)

set(io_systems_sources
    src/io/systems/FtdiUsbSystem.cpp
    src/io/systems/FtdiUsbSystem.h
)

set(properties_sources
    src/properties/BaseSensorPropertiesV0.h
    src/properties/BaseSensorPropertiesV1.h
    src/properties/CorePropertyRulesV1.h
    src/properties/ImuPropertyRulesV1.h
    src/properties/ImuPropertyRulesV2.h
    src/properties/ImuSensorPropertiesV0.h
    src/properties/ImuSensorPropertiesV1.h
    src/properties/LegacyCoreProperties.cpp
    src/properties/LegacyCoreProperties.h
    src/properties/Ig1CoreProperties.cpp
    src/properties/Ig1CoreProperties.h
    src/properties/LegacyImuProperties.cpp
    src/properties/LegacyImuProperties.h
    src/properties/Ig1ImuProperties.cpp
    src/properties/Ig1ImuProperties.h
    src/properties/Ig1GnssProperties.cpp
    src/properties/Ig1GnssProperties.h
)

set(utility_sources
    src/utility/Finally.h
    src/utility/IPlatformDll.h
    src/utility/LockingQueue.h
    src/utility/Ownership.h
    src/utility/ReferenceCmp.h
    src/utility/StringView.h
    src/utility/ThreadFence.h
)

if(ZEN_BLUETOOTH)
    set(io_ble_sources
        src/io/ble/BleDeviceFinder.cpp
        src/io/ble/BleDeviceFinder.h
        src/io/ble/BleDeviceHandler.cpp
        src/io/ble/BleDeviceHandler.h
    )

    set(io_bluetooth_sources
        src/io/bluetooth/BluetoothDeviceFinder.cpp
        src/io/bluetooth/BluetoothDeviceFinder.h
        src/io/bluetooth/BluetoothDeviceHandler.cpp
        src/io/bluetooth/BluetoothDeviceHandler.h
    )

    list (APPEND io_systems_sources
        src/io/systems/BleSystem.cpp
        src/io/systems/BleSystem.h
        src/io/systems/BluetoothSystem.cpp
        src/io/systems/BluetoothSystem.h
    )

    list (APPEND io_interfaces_sources
        src/io/interfaces/BleInterface.cpp
        src/io/interfaces/BleInterface.h
        src/io/interfaces/BluetoothInterface.cpp
        src/io/interfaces/BluetoothInterface.h
    )

    list (APPEND zen_optional_libs
        Qt5::Bluetooth
    )

    list (APPEND zen_optional_compile_definitions_private
        ZEN_BLUETOOTH
    )

else()
    set(io_ble_sources)
    set(io_bluetooth_sources)
endif()


if(WIN32)

    set(io_can_sources ${io_can_sources}
        src/io/can/PcanBasicChannel.cpp
        src/io/can/PcanBasicChannel.h
    )
    set(io_interfaces_sources ${io_interfaces_sources}
        src/io/interfaces/SiUsbInterface.cpp
        src/io/interfaces/SiUsbInterface.h
        src/io/interfaces/windows/WindowsDeviceInterface.cpp
        src/io/interfaces/windows/WindowsDeviceInterface.h
    )

    set(io_systems_sources ${io_systems_sources}
        src/io/systems/PcanBasicSystem.cpp
        src/io/systems/PcanBasicSystem.h
        src/io/systems/SiUsbSystem.cpp
        src/io/systems/SiUsbSystem.h
        src/io/systems/windows/EnumerateSerialPorts.cpp
        src/io/systems/windows/EnumerateSerialPorts.h
        src/io/systems/windows/WindowsDeviceSystem.cpp
        src/io/systems/windows/WindowsDeviceSystem.h
    )

    set(utility_sources ${utility_sources}
        src/utility/windows/FindThisModule.cpp
        src/utility/windows/FindThisModule.h
        src/utility/windows/WindowsDll.cpp
        src/utility/windows/WindowsDll.h
    )

elseif(UNIX AND NOT APPLE)

    set(io_interfaces_sources ${io_interfaces_sources}
        src/io/interfaces/linux/LinuxDeviceInterface.cpp
        src/io/interfaces/linux/LinuxDeviceInterface.h
    )

    set(io_systems_sources ${io_systems_sources}
        src/io/systems/linux/LinuxDeviceSystem.cpp
        src/io/systems/linux/LinuxDeviceSystem.h
    )

    set(utility_sources ${utility_sources}
        src/utility/linux/LinuxDll.cpp
        src/utility/linux/LinuxDll.h
    )

else()
    message(FATAL_ERROR "OS not supported.")
endif()

set(zen_all_sources
    ${zen_includes}
    ${zen_sources}
    ${communication_sources}
    ${components_sources}
    ${components_factories_sources}
    ${io_sources}
    ${io_ble_sources}
    ${io_bluetooth_sources}
    ${io_can_sources}
    ${io_interfaces_sources}
    ${io_systems_sources}
    ${properties_sources}
    ${utility_sources}
)

if(ZEN_USE_STATIC_LIBS)
    add_library(OpenZen STATIC ${zen_all_sources})
else()
    add_library(OpenZen SHARED ${zen_all_sources})
endif()

target_compile_definitions(OpenZen
    PRIVATE
        $<IF:$<BOOL:${ZEN_USE_STATIC_LIBS}>,ZEN_API_STATIC,ZEN_API_EXPORT>
        ${zen_optional_compile_definitions_private}
)

target_compile_features(OpenZen
    PRIVATE
        cxx_std_17
)

if (MSVC)
    target_compile_options(OpenZen
        PRIVATE
            /W4 /WX
    )
else()
    target_compile_options(OpenZen
        PRIVATE
            -Wall -Wextra -pedantic
    )

    # add linker options to statically compile in libstdc++
    # this makes the library portable to older Linux versions
    if (ZEN_STATIC_LINK_LIBCXX)
        list (APPEND zen_optional_libs
            -static-libstdc++
            -static-libgcc
        )
    endif()
endif()

set(zen_include_dirs_public
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(zen_include_dirs_private
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<TARGET_PROPERTY:ftdi,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:GSL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:nonstd::expected-lite,INTERFACE_INCLUDE_DIRECTORIES>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:pcanbasic,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:siusb,INTERFACE_INCLUDE_DIRECTORIES>>
)

target_include_directories(OpenZen
    PUBLIC
        ${zen_include_dirs_public}
    PRIVATE
        ${zen_include_dirs_private}
)

include(FindThreads)

set (zen_libs
        $<$<PLATFORM_ID:Linux>:atomic>
        $<$<PLATFORM_ID:Linux>:dl>
        nlohmann_json::nlohmann_json
        $<$<PLATFORM_ID:Linux>:rt>
        $<$<PLATFORM_ID:Linux>:stdc++fs>
        Threads::Threads
        spdlog
        ${zen_optional_libs}
)

target_link_libraries(OpenZen
    PRIVATE
        ${zen_libs}
)

# Unit test binary for OpenZen
#
# This will build the OpenZen source files into a binary
# and compile the unit test code. The DLL version of OpenZen cannot be used
# because the most of the symbols we want to test are not visible with the
# Windows DLL default visibility.
add_executable(OpenZenTests
  ${zen_all_sources}
  src/test/ModbusTest.cpp
  src/test/communication/ConnectionNegotiatorTest.cpp
  src/test/OpenZenTests.cpp)

target_include_directories(OpenZenTests
    PUBLIC
        ${zen_include_dirs_public}
    PRIVATE
        ${zen_include_dirs_private}
)

target_link_libraries(OpenZenTests
    PRIVATE
        gtest
        ${zen_libs}
)

target_compile_features(OpenZenTests
    PRIVATE
        cxx_std_17
)

# always compile the unit test binary without any function exports
target_compile_definitions(OpenZenTests
    PRIVATE
        ZEN_API_STATIC
        ${zen_optional_compile_definitions_private}
)

# build example code
add_subdirectory(examples)

# configuration for CMake package installation
set_property(TARGET OpenZen PROPERTY PUBLIC_HEADER
    include/OpenZen.h
    include/OpenZenCAPI.h
    include/ZenProtocol.h
    include/ZenTypes.h
)

if (ZEN_INSTALL)
    install(TARGETS OpenZen EXPORT OpenZenTarget
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include
    )

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/OpenZenConfigVersion.cmake"
        VERSION ${OpenZen_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    export(EXPORT OpenZenTarget
        FILE "${CMAKE_CURRENT_BINARY_DIR}/OpenZenConfigTargets.cmake"
        NAMESPACE OpenZen::
    )
    configure_file(cmake/OpenZenConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/OpenZenConfig.cmake"
        COPYONLY
    )

    set(ConfigPackageLocation lib/cmake/OpenZen)
    install(EXPORT OpenZenTarget
        FILE
            OpenZenTargets.cmake
        NAMESPACE
            OpenZen::
        DESTINATION
            ${ConfigPackageLocation}
    )
    install(
        FILES
            cmake/OpenZenConfig.cmake
            "${CMAKE_CURRENT_BINARY_DIR}/OpenZenConfigVersion.cmake"
        DESTINATION
            ${ConfigPackageLocation}
        COMPONENT
            Devel
    )
endif()