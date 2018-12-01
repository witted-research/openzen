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
    include/IZenSensor.h
    include/IZenSensorManager.h
    include/ZenTypes.h
)

set(zen_sources
    src/ImuHelpers.h
    src/InternalTypes.h
    src/SensorManager.cpp
    src/SensorManager.h
    
    src/LpMatrix.c
    src/LpMatrix.h
)

set(io_sources
    src/io/BleDeviceFinder.cpp
    src/io/BleDeviceFinder.h
    src/io/BleDeviceHandler.cpp
    src/io/BleDeviceHandler.h
    src/io/IIoInterface.h
    src/io/IIoSystem.h
    src/io/IoManager.cpp
    src/io/IoManager.h
    src/io/Modbus.cpp
    src/io/Modbus.h
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
    src/io/interfaces/BaseIoInterface.cpp
    src/io/interfaces/BaseIoInterface.h
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
    src/properties/ImuSensorPropertiesV0.h
)

set(sensors_sources
    src/sensors/BaseSensor.cpp
    src/sensors/BaseSensor.h
    src/sensors/IMUSensor.cpp
    src/sensors/IMUSensor.h
)

set(utility_sources
    src/utility/Finally.h
    src/utility/IPlatformDll.h
    src/utility/LockingQueue.h
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

if(ZEN_USE_STATIC_LIBS)
    add_library(OpenZen STATIC
        ${zen_includes}
        ${zen_sources}
        ${io_sources}
        ${io_bluetooth_sources}
        ${io_can_sources}
        ${io_interfaces_sources}
        ${io_systems_sources}
        ${properties_sources}
        ${sensors_sources}
        ${utility_sources}
    )
else()
    add_library(OpenZen SHARED
        ${zen_includes}
        ${zen_sources}
        ${io_sources}
        ${io_bluetooth_sources}
        ${io_can_sources}
        ${io_interfaces_sources}
        ${io_systems_sources}
        ${properties_sources}
        ${sensors_sources}
        ${utility_sources}
    )
endif()

target_compile_definitions(OpenZen
    PRIVATE
        $<IF:$<BOOL:${ZEN_USE_STATIC_LIBS}>,ZEN_API_STATIC,ZEN_API_EXPORT>
)

target_compile_features(OpenZen
    PRIVATE
        cxx_std_17
)

target_include_directories(OpenZen
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:ftdi,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:pcanbasic,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:siusb,INTERFACE_INCLUDE_DIRECTORIES>>
)

include(FindThreads)

target_link_libraries(OpenZen
    PRIVATE
        $<$<PLATFORM_ID:Linux>:atomic>
        $<$<PLATFORM_ID:Linux>:dl>
        Qt5::Bluetooth
        $<$<PLATFORM_ID:Linux>:stdc++fs>
        Threads::Threads
)

add_subdirectory(examples)