cmake_minimum_required(VERSION 3.11)
project(OpenZen VERSION 0.1.0 LANGUAGES CXX;C)

add_subdirectory(external)

option(ZEN_USE_STATIC_LIBS "Whether to compile OpenZen as a static library" OFF)

#----------------------------------------------------------------
# Packages
#----------------------------------------------------------------
# Find includes in corresponding build include_directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)

find_package(Qt5Bluetooth REQUIRED)

set(zen_includes
    include/OpenZen.h
    include/OpenZenCAPI.h
    include/ZenProtocol.h
    include/ZenTypes.h
)

set(zen_sources
    src/ImuHelpers.h
    src/InternalTypes.h
    src/ISensorProperties.h
    src/OpenZen.cpp
    src/Sensor.cpp
    src/Sensor.h
    src/SensorConfig.h
    src/SensorComponent.h
    src/SensorManager.cpp
    src/SensorManager.h
    src/SensorProperties.cpp
    src/SensorProperties.h
    
    src/LpMatrix.c
    src/LpMatrix.h
)

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
)

set(components_factories_sources
    src/components/factories/ImuComponentFactory.cpp
    src/components/factories/ImuComponentFactory.h
)

set(io_sources
    src/io/IIoInterface.h
    src/io/IIoSystem.h
    src/io/IoManager.cpp
    src/io/IoManager.h
)

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

set(io_can_sources
    src/io/can/CanManager.cpp
    src/io/can/CanManager.h
    src/io/can/ICanChannel.h
)

set(io_interfaces_sources
    src/io/interfaces/BleInterface.cpp
    src/io/interfaces/BleInterface.h
    src/io/interfaces/BluetoothInterface.cpp
    src/io/interfaces/BluetoothInterface.h
    src/io/interfaces/CanInterface.cpp
    src/io/interfaces/CanInterface.h
    src/io/interfaces/FtdiUsbInterface.cpp
    src/io/interfaces/FtdiUsbInterface.h
)

set(io_systems_sources
    src/io/systems/BleSystem.cpp
    src/io/systems/BleSystem.h
    src/io/systems/BluetoothSystem.cpp
    src/io/systems/BluetoothSystem.h
    src/io/systems/FtdiUsbSystem.cpp
    src/io/systems/FtdiUsbSystem.h
)

set(properties_sources
    src/properties/BaseSensorPropertiesV0.h
    src/properties/CorePropertyRulesV1.h
    src/properties/ImuPropertyRulesV1.h
    src/properties/ImuSensorPropertiesV0.h
    src/properties/LegacyCoreProperties.cpp
    src/properties/LegacyCoreProperties.h
    src/properties/LegacyImuProperties.cpp
    src/properties/LegacyImuProperties.h
)

set(utility_sources
    src/utility/Finally.h
    src/utility/IPlatformDll.h
    src/utility/LockingQueue.h
    src/utility/Ownership.h
    src/utility/StringView.h
    src/utility/ThreadFence.h
)

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
        src/io/systems/windows/WindowsDeviceSystem.cpp
        src/io/systems/windows/WindowsDeviceSystem.h
    )

    set(utility_sources ${utility_sources}
        src/utility/windows/WindowsDll.cpp
        src/utility/windows/WindowsDll.h
    )

elseif(UNIX AND NOT APPLE)

    set(utility_sources ${utility_sources}
        src/utility/linux/LinuxDll.cpp
        src/utility/linux/LinuxDll.h
    )

else()
    message(FATAL_ERROR "OS not supported.")
endif()

set(all_sources
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
    add_library(OpenZen STATIC ${all_sources})
else()
    add_library(OpenZen SHARED ${all_sources})
endif()

target_compile_definitions(OpenZen
    PRIVATE
        $<IF:$<BOOL:${ZEN_USE_STATIC_LIBS}>,ZEN_API_STATIC,ZEN_API_EXPORT>
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
            -Wall -Wextra -pedantic -Werror
    )
endif()

target_include_directories(OpenZen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<TARGET_PROPERTY:ftdi,INTERFACE_INCLUDE_DIRECTORIES>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:pcanbasic,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:siusb,INTERFACE_INCLUDE_DIRECTORIES>>
)

include(FindThreads)

target_link_libraries(OpenZen
    PRIVATE
        $<$<PLATFORM_ID:Linux>:atomic>
        $<$<PLATFORM_ID:Linux>:dl>
        nonstd::expected-lite
        nlohmann_json::nlohmann_json
        GSL
        Qt5::Bluetooth
        $<$<PLATFORM_ID:Linux>:stdc++fs>
        Threads::Threads
)

add_subdirectory(examples)
