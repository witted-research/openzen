# OpenZen

High performance sensor data streaming and processing

Full documentation: <https://lpresearch.bitbucket.io/openzen/>

![OpenZen Unity plugin connected to a LPMS-CU2 sensor and live visualization of sensor orientation](https://lpresearch.bitbucket.io/openzen_resources/OpenZenUnityDemo.gif)

OpenZen Unity plugin connected to a LPMS-CU2 sensor and live visualization of sensor orientation.

## Overview
OpenZen is a framework that allows access to Sensor Hardware from multiple vendors without requiring understanding of the target hardware or communication layer by providing a homogeneous API to application developers and a communication protocol to hardware developers.

Currently, OpenZen has full support for communication over Bluetooth and USB (using the Silabs driver), as well as experimental support for Bluetooth Low-Energy, CAN (using the PCAN Basic driver) and USB (using the Ftdi driver). At the moment OpenZen has only been used with IMU sensors, but is easily extensible to any type of sensor or range of properties.

Where possible we favour open standards, but as OpenZen finds its origin in [OpenMAT](https://bitbucket.org/lpresearch/openmat-2-os/) it needs to provide backwards compatibility for all sensors that originally worked with its *LpSensor* library. As such, OpenZen still contains some custom protocols which will be phased out over time. Legacy protocols are generally referred to with the v0 indicator.

You can find the full documentation for OpenZen here: <https://lpresearch.bitbucket.io/openzen/>

## Download Library

You can download the newest pre-compiled version of OpenZen for your platform here:

<https://bitbucket.org/lpresearch/openzen/downloads/>

## Setup

OpenZen uses CMake (3.11 or higher) as build system, and has been built and tested on Windows (MSVC) and Ubuntu (gcc7). As OpenZen is written in C++17, an up-to-date compiler is required. The following steps guide you through setting up OpenZen in Windows and Linux.

### Windows

1. Install MSVC build tools, or the Visual Studio IDE (requires C++17 support)
2. Install CMake, or a GUI (e.g. Visual Studio) that incorporates CMake
3. Install Qt (5.11.2 or higher)
4. Clone the external repositories: `git submodule update --init`
5. Configure CMake with the environment variable `CMAKE_PREFIX_PATH` pointing towards your Qt bin directory

You can do that by inserting this line (with the correct Qt installation path on your system)
```
set(CMAKE_PREFIX_PATH "C://Qt//5.12.3//msvc2017_64//")
```
in the topmost CMakeLists.txt

6. Compile and run the *OpenZenExample* using MSVC

### Linux

1. Install gcc7 (requires C++17 support): `sudo apt-get install gcc-7`
2. Install CMake ([instructions](https://peshmerge.io/how-to-install-cmake-3-11-0-on-ubuntu-16-04/))
3. Install Qt (5.11.2 or higher): `sudo apt-get install qtbase5-dev qtconnectivity5-dev`
4. Clone the external repositories: `git submodule update --init`
5. Create a build folder and run cmake:
```
mkdir build && cd build
cmake ..
```
6. Compile and run the *OpenZenExample*: 
```
make -j4
examples/OpenZenExample
```

An example of how to use the OpenZen API is included with the repository. If you are looking for more information on how to use the API, visit the documentation on the [Wiki](https://bitbucket.org/lpresearch/openzen/wiki/API%20Documentation).

## Deployment

If you want to compile OpenZen and use the binary library on other systems, you can use CMake for that too. To build a standlone version of OpenZen, you can use the following command:

```
cmake -DCMAKE_BUILD_TYPE=Release -DZEN_STATIC_LINK_LIBCXX=On -DZEN_BLUETOOTH=OFF -DCMAKE_INSTALL_PREFIX=../OpenZenRelease/ ..
make -j4 install
```

This will compile OpenZen without Bluetooth support (if you don't need it) which makes the library independant of any Qt libraries. Furthermore, it will statically link libstdc++ which increase the size of the library bigger but makes it more portable.
After calling `make install`, the folder `OpenZenRelease` will contain the binary library, the required interface header files and CMake configuration file to easily integrate the library into your project.

Please see this [CMake file](https://bitbucket.org/lpresearch/openzen/src/master/standalone_example/CMakeLists.txt) for an example how to use OpenZen as an external, binary-only package in your build.

## Note on Licensing

OpenZen is licensed under a simple MIT-type license and thus is free to use and modify for all purposes.  Please be aware
that OpenZen relies on the Qt library for Bluetooth functionality and thus Qt's license conditions may apply to the resulting
binary. If you want to avoid this, OpenZen can be built without Bluetooth support and thus without using Qt. This is
achieved by setting the CMake option `ZEN_BLUETOOTH` to `OFF`, e.g. by adding `-DZEN_BLUETOOTH=OFF` to the command line while
configuring.

## Contributing

We welcome any contributions to OpenZen through pull requests. The most up-to-date builds are on the Master branch, so we prefer to take pull requests there. There is no official coding standard, but we ask you to at least adhere to the following guidelines:

* Use spaces instead of tabs
* Put braces on a new line
* Use upper camel case for class names: `FooBar`
* Use camel case for function names: `fooBar()`
* Use camel case for variable names: `fooBar`
* Prepend `m_` to member variables: `m_fooBar`
* Prepend `g_` to global variables: `g_fooBar`

In general, we ask you to prefer legible code with descriptive variable names over comments, but to not shy away from using commands where necessary. Apart from this you are free to write code the way you like it. It is however left to the code reviewers judgement whether your pull request complies with the standards of OpenZen.
