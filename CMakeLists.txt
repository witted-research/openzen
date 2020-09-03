cmake_minimum_required(VERSION 3.10)
project(OpenZen VERSION 1.0.0 LANGUAGES CXX;C)

option(ZEN_MSVC_RUNTIME_STATIC "Link the MSVC runtime statically, useful for library releases" OFF)

# Global compile options which also affect the externals
if (MSVC)
    # make sure we link with the static runtime libraries
    # and have no dependencies on VC++ studio DLLs during runtime
    # We don't have any C++ exposed, so the C++ compatibility
    # to the host program of the DLL is not required

    # according to
    # https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#how-can-i-build-my-msvc-application-with-a-static-runtime
    if (ZEN_MSVC_RUNTIME_STATIC)
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
           if(${flag_var} MATCHES "/MD")
              string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
           endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif()

    # select windows 7 as base version
    # https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt?view=vs-2019
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-D_WINVER=0x0601)
endif()

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    message(FATAL_ERROR "GCC Version 7.0 is at least need to compile OpenZen")
endif()

set (ZEN_BLUETOOTH_DEFAULT ON)

## check if libbluetooth is available on non-Apple Unix
if(UNIX AND NOT APPLE)
    find_library(LIBBLUETOOTH_LIBRARY
        NAMES bluetooth)

    if (NOT LIBBLUETOOTH_LIBRARY)
        set(ZEN_BLUETOOTH_DEFAULT OFF)

        message("Please install libbluetooth-dev for bluetooth support")
        if (ZEN_BLUETOOTH)
            # give a fatal error message if Bluetooth support was requested
            # by cmake parameters but we cannot fullfil it
            message(FATAL_ERROR "libbluetooth-dev required to compile with Bluetooth support")
        endif()
    endif()
endif()

# Please note: The static OpenZen build does not support library installation (aka "make install")
# as the private dependencies cannot be resolved by CMake
# see https://gitlab.kitware.com/cmake/cmake/issues/17357 for example
option(ZEN_USE_STATIC_LIBS "Whether to compile OpenZen as a static library" OFF)
option(ZEN_STATIC_LINK_LIBCXX "Option to statically link libstdc++ to be portable to older systems (Linux only)" OFF)
# linux needs libbluetooth-dev to be able to compile, so disable it by default
option(ZEN_BLUETOOTH "Compile OpenZen with bluetooth support" ${ZEN_BLUETOOTH_DEFAULT})
option(ZEN_BLUETOOTH_BLE "Compile OpenZen with bluetooth low-energy support, needs Qt installed" OFF)
option(ZEN_NETWORK "Compile OpenZen with support for network streaming of measurement data" OFF)
option(ZEN_CSHARP "Compile C# bindings for OpenZen" ON)
option(ZEN_PYTHON "Compile Python bindings for OpenZen" OFF)
option(ZEN_TESTS "Compile with OpenZen tests" ON)
option(ZEN_EXAMPLES "Compile with OpenZen examples" ON)
option(ZEN_USE_BINARY_LIBRARIES "If set to true, binaries libraries are downloaded during the build" ON)

# Enable this line and set it to the correct Qt installation path if you want to compile
# with bluetooth support on Windows
#set(CMAKE_PREFIX_PATH "C://Qt//5.12.3//msvc2017_64//")

#----------------------------------------------------------------
# Packages
#----------------------------------------------------------------
# Find includes in corresponding build include_directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(external)

# contains additional C/C++ precompiler defines
# which might be set depending on the selected compile options
set(zen_optional_compile_definitions_private)
set(zen_optional_libs)

# set the correct log level
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # this will enable the SPDLOG_DEBUG macros for the build
    list (APPEND zen_optional_compile_definitions_private
        SPDLOG_ACTIVE_LEVEL=1
    )
endif()

if (ZEN_BLUETOOTH_BLE)
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
        # swig.exe -csharp -small -c++ -debug-typedef -o OpenZenCSharp/OpenZen_wrap_csharp.cxx -outdir OpenZenCSharp OpenZen.i
        # in the bindings folder to generate
        bindings/OpenZenCSharp/OpenZen_wrap_csharp.cxx
    )
endif()

if(ZEN_PYTHON)
    # this is only available in cmake 3.12 or newer
    # look and build only for python3
    find_package (Python3 REQUIRED COMPONENTS Development)
    list (APPEND zen_sources
        ## run
        # swig.exe -python -small -c++ -debug-typedef -o OpenZenPython/OpenZen_wrap_python.cxx -outdir OpenZenPython OpenZen.i
        # in the bindings folder to generate
        bindings/OpenZenPython/OpenZen_wrap_python.cxx
    )
    list (APPEND zen_optional_libs Python3::Python)
endif()

set(zen_optional_test_sources)

set(processors_sources
    src/processors/DataProcessor.h
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
    src/components/ImuIg1Component.cpp
    src/components/ImuIg1Component.h
    src/components/GnssComponent.cpp
    src/components/GnssComponent.h
    src/components/SensorParsingUtil.h
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
    src/io/interfaces/TestSensorInterface.cpp
    src/io/interfaces/TestSensorInterface.h
)

set(io_systems_sources
    src/io/systems/TestSensorSystem.cpp
    src/io/systems/TestSensorSystem.h
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
    src/utility/gnss/RTCM3NetworkSource.h
    src/utility/gnss/RTCM3NetworkSource.cpp
    src/utility/gnss/RTCM3SerialSource.h
    src/utility/gnss/RTCM3SerialSource.cpp
    src/utility/gnss/RTCM3Parser.h
)

if(ZEN_BLUETOOTH_BLE)
    set(io_ble_sources
        src/io/ble/BleDeviceFinder.cpp
        src/io/ble/BleDeviceFinder.h
        src/io/ble/BleDeviceHandler.cpp
        src/io/ble/BleDeviceHandler.h
    )

    list (APPEND io_systems_sources
        src/io/systems/BleSystem.cpp
        src/io/systems/BleSystem.h
    )

    list (APPEND io_interfaces_sources
        src/io/interfaces/BleInterface.cpp
        src/io/interfaces/BleInterface.h
    )

    list (APPEND zen_optional_libs
        Qt5::Bluetooth
    )

    list (APPEND zen_optional_compile_definitions_private
        ZEN_BLUETOOTH_BLE
    )
endif()

if(ZEN_BLUETOOTH)
    set(io_bluetooth_sources
        src/utility/bluetooth-serial-port/BluetoothException.h
        src/utility/bluetooth-serial-port/BTSerialPortBinding.h
        src/utility/bluetooth-serial-port/DeviceINQ.h
        src/utility/bluetooth-serial-port/Enums.h
        src/utility/bluetooth-serial-port/Enums.cc
        src/io/bluetooth/BluetoothDeviceFinder.cpp
        src/io/bluetooth/BluetoothDeviceFinder.h
        src/io/bluetooth/BluetoothDeviceHandler.cpp
        src/io/bluetooth/BluetoothDeviceHandler.h
    )

    if(WIN32) # windows
        list (APPEND io_bluetooth_sources
            src/utility/bluetooth-serial-port/windows/BluetoothHelpers.cc
            src/utility/bluetooth-serial-port/windows/BTSerialPortBinding.cc
            src/utility/bluetooth-serial-port/windows/DeviceINQ.cc
        )

        list (APPEND zen_optional_libs
            ws2_32
            bthprops
        )
    elseif(APPLE) # MacOSX
        list (APPEND io_bluetooth_sources
            src/utility/bluetooth-serial-port/osx/BluetoothDeviceResources.mm
            src/utility/bluetooth-serial-port/osx/BluetoothWorker.mm
            src/utility/bluetooth-serial-port/osx/BTSerialPortBinding.mm
            src/utility/bluetooth-serial-port/osx/DeviceINQ.mm
            src/utility/bluetooth-serial-port/osx/pipe.c
        )
        find_library(FOUNDATION Foundation)
        find_library(IOBLUETOOTH IOBluetooth)
        list (APPEND zen_optional_libs
            ${FOUNDATION}
            ${IOBLUETOOTH}
            -fobjc-arc
        )

        add_executable(btScan src/utility/bluetooth-serial-port/osx/btScan.mm)
        target_link_libraries(btScan ${FOUNDATION} ${IOBLUETOOTH})
        # Use a non-ancient C++ version for compilation ...
        target_compile_features(btScan PRIVATE cxx_variadic_templates)
    else() # Linux
        list (APPEND io_bluetooth_sources
            src/utility/bluetooth-serial-port/linux/BTSerialPortBinding.cc
            src/utility/bluetooth-serial-port/linux/DeviceINQ.cc
        )

        list (APPEND zen_optional_libs
            bluetooth
        )
    endif()

    list (APPEND io_systems_sources
        src/io/systems/BluetoothSystem.cpp
        src/io/systems/BluetoothSystem.h
    )

    list (APPEND io_interfaces_sources
        src/io/interfaces/BluetoothInterface.cpp
        src/io/interfaces/BluetoothInterface.h
    )

    list (APPEND zen_optional_compile_definitions_private
        ZEN_BLUETOOTH
    )
endif()

if(ZEN_NETWORK)
    set(io_zeromq_sources
        src/io/interfaces/ZeroMQInterface.h
        src/io/interfaces/ZeroMQInterface.cpp
    )

    list (APPEND io_systems_sources
        src/io/systems/ZeroMQSystem.h
        src/io/systems/ZeroMQSystem.cpp
    )

    list (APPEND zen_optional_test_sources
        src/test/streaming/ZeroMQStreamingTest.cpp
    )

    list (APPEND processors_sources
        src/processors/ZmqDataProcessor.h
        src/processors/ZmqDataProcessor.cpp
    )

    list (APPEND zen_optional_libs
        libzmq-static
        cppzmq
    )

    list (APPEND zen_optional_compile_definitions_private
        ZEN_NETWORK
    )

else()
    set(io_zeromq_sources)
endif()

if(WIN32)

    set(io_interfaces_sources ${io_interfaces_sources}
        src/io/interfaces/windows/WindowsDeviceInterface.cpp
        src/io/interfaces/windows/WindowsDeviceInterface.h
    )

    set(io_systems_sources ${io_systems_sources}
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

    # add drivers which need binary libraries
    if (ZEN_USE_BINARY_LIBRARIES)
        set(io_can_sources ${io_can_sources}
            src/io/can/PcanBasicChannel.cpp
            src/io/can/PcanBasicChannel.h
        )

        list (APPEND io_interfaces_sources
            src/io/interfaces/SiUsbInterface.cpp
            src/io/interfaces/SiUsbInterface.h
            src/io/interfaces/FtdiUsbInterface.cpp
            src/io/interfaces/FtdiUsbInterface.h
        )

        list (APPEND io_systems_sources
            src/io/systems/PcanBasicSystem.cpp
            src/io/systems/PcanBasicSystem.h
            src/io/systems/SiUsbSystem.cpp
            src/io/systems/SiUsbSystem.h
            src/io/systems/FtdiUsbSystem.cpp
            src/io/systems/FtdiUsbSystem.h
        )

        list (APPEND zen_optional_compile_definitions_private
            ZEN_USE_BINARY_LIBRARIES
        )
    endif()

elseif(UNIX AND NOT APPLE)

    set(io_interfaces_sources ${io_interfaces_sources}
        src/io/interfaces/posix/PosixDeviceInterface.cpp
        src/io/interfaces/posix/PosixDeviceInterface.h
    )

    set(io_systems_sources ${io_systems_sources}
        src/io/systems/linux/LinuxDeviceSystem.cpp
        src/io/systems/linux/LinuxDeviceSystem.h
    )

    set(utility_sources ${utility_sources}
        src/utility/posix/PosixDll.cpp
        src/utility/posix/PosixDll.h
    )

elseif(APPLE)

    set(io_interfaces_sources ${io_interfaces_sources}
        src/io/interfaces/posix/PosixDeviceInterface.cpp
        src/io/interfaces/posix/PosixDeviceInterface.h
    )

    set(io_systems_sources ${io_systems_sources}
        src/io/systems/mac/MacDeviceSystem.cpp
        src/io/systems/mac/MacDeviceSystem.h
    )

    set(utility_sources ${utility_sources}
        src/utility/posix/PosixDll.cpp
        src/utility/posix/PosixDll.h
    )
else()
    message(FATAL_ERROR "OS not supported.")
endif()

set(zen_all_sources
    ${zen_includes}
    ${zen_sources}
    ${processors_sources}
    ${communication_sources}
    ${components_sources}
    ${components_factories_sources}
    ${io_sources}
    ${io_ble_sources}
    ${io_bluetooth_sources}
    ${io_zeromq_sources}
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

    # hide all non-exported symbols in the build
    set_target_properties(OpenZen PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(OpenZen PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
endif()

target_compile_definitions(OpenZen
    PRIVATE
        ${zen_optional_compile_definitions_private}
)

# publish the static usage flag of OpenZen to all libraries linking to us
if (ZEN_USE_STATIC_LIBS)
    target_compile_definitions(OpenZen PUBLIC ZEN_API_STATIC)
else()
    target_compile_definitions(OpenZen PRIVATE ZEN_API_EXPORT)
endif()

target_compile_features(OpenZen
    PRIVATE
        cxx_std_17
)

set_property(TARGET OpenZen PROPERTY CXX_EXTENSIONS FALSE)

if (MSVC)
    target_compile_options(OpenZen
        PRIVATE
            /W4
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
        $<TARGET_PROPERTY:GSL,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:openzen_asio,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:cereal,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:nonstd::expected-lite,INTERFACE_INCLUDE_DIRECTORIES>
)

if (ZEN_USE_BINARY_LIBRARIES)
    set(zen_include_dirs_private ${zen_include_dirs_private}
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:pcanbasic,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:siusb,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<PLATFORM_ID:Windows>:$<TARGET_PROPERTY:ftdi,INTERFACE_INCLUDE_DIRECTORIES>>
    )
endif()

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

# copy library to a name with underscore to conform with python naming conventions
# http://www.swig.org/Doc1.3/Python.html#Python_nn7
#message(FATAL_ERROR "Will copy from $<TARGET_FILE:OpenZen> to ${CMAKE_CURRENT_BINARY_DIR}/lib/_OpenZen.pyd")
#message(FATAL_ERROR "Will ${ZEN_PYTHON} and ${WIN32}")

if (ZEN_PYTHON)
    if(WIN32)
        # rename file to be used as python module in Windows
        #message(FATAL_ERROR "Will copy from ${<TARGET_FILE:OpenZen>} to ${CMAKE_CURRENT_BINARY_DIR}/lib/_OpenZen.pyd")
        add_custom_command(TARGET OpenZen POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:OpenZen> ${CMAKE_CURRENT_BINARY_DIR}/_OpenZen.pyd
        )
    endif()

    if(UNIX)
        # rename file to be used as python module in Linux/Max
        add_custom_command(TARGET OpenZen POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:OpenZen> ${CMAKE_CURRENT_BINARY_DIR}/_OpenZen.so
        )
    endif()

    # copy python interface file
    add_custom_command(TARGET OpenZen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bindings/OpenZenPython/OpenZen.py ${CMAKE_CURRENT_BINARY_DIR}/OpenZen.py
    )
endif()

if (ZEN_TESTS)
    # Unit test binary for OpenZen
    #
    # This will build the OpenZen source files into a binary
    # and compile the unit test code. The DLL version of OpenZen cannot be used
    # because the most of the symbols we want to test are not visible with the
    # Windows DLL default visibility.
    add_executable(OpenZenTests
    ${zen_all_sources}
    ${zen_optional_test_sources}
    src/test/ModbusTest.cpp
    src/test/communication/ConnectionNegotiatorTest.cpp
    src/test/components/GnssComponentTest.cpp
    src/test/streaming/SerializationTest.cpp
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
endif()

if (ZEN_EXAMPLES)
    # build example code
    add_subdirectory(examples)
endif()

# configuration for CMake package installation
set_property(TARGET OpenZen PROPERTY PUBLIC_HEADER
    include/OpenZen.h
    include/OpenZenCAPI.h
    include/ZenProtocol.h
    include/ZenTypes.h
)

install(TARGETS OpenZen EXPORT OpenZenTarget
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

if(WIN32 AND ZEN_USE_BINARY_LIBRARIES)
    get_target_property(FTDI_DLL ftdi IMPORTED_LOCATION)
    get_target_property(SIUSB_DLL siusb IMPORTED_LOCATION)
    get_target_property(PCANBASIC_DLL pcanbasic IMPORTED_LOCATION)

    install(FILES ${FTDI_DLL} ${SIUSB_DLL} ${PCANBASIC_DLL} DESTINATION bin)
endif()

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
