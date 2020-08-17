#####
Setup
#####

..
   The following gives us :bash:`highlight as if this were a bash command.`

.. role:: bash(code)
    :language: bash

Dowloading pre-compiled OpenZen
===============================

The pre-compiled binaries for OpenZen for Windows and Linux are available to download here:

`OpenZen Releases <https://bitbucket.org/lpresearch/openzen/downloads/>`_

The folder example in this download contains a full example how to use OpenZen
together with VisualStudio 2017 on Windows x64-bit. Furthermore, it also contains releases
of OpenZen for Ubuntu 16.04 (and newer) with support for x64 and ARM64.

Building OpenZen from Source
============================

OpenZen uses CMake (3.11 or higher) as build system, and has been built
and tested on Windows (MSVC), Ubuntu (gcc7) and Mac (XCode). As OpenZen is written in
C++17, an up-to-date compiler is required. The following steps guide you
through building OpenZen for Windows and Linux.

Windows
-------

- Install MSVC build tools, or the Visual Studio IDE (requires C++17 support)
- Install CMake, or a GUI (e.g. Visual Studio) that incorporates CMake
- Clone the repository:

.. code-block:: bash

    git clone --recurse-submodules https://bitbucket.org/lpresearch/openzen.git

Open the folder OpenZen in Visual Studio and compile the library. You will then be
able to run the the command line tool OpenZenExample in the output folder of Visual Studio.

Linux
-----

- Install gcc7 or higher (requires C++17 support): :bash:`sudo apt-get install gcc-7`
- Install CMake, git: :bash:`sudo apt-get install git cmake`
- Install the Bluetooth libraries if you need Bluetooth support:
  :bash:`sudo apt-get install libbluetooth-dev`
- Install Qt (5.11.2 or higher) if you need Bluetooth Low Energy support:
  :bash:`sudo apt-get install qtbase5-dev qtconnectivity5-dev`
- Clone the repository:

.. code-block:: bash

    git clone --recurse-submodules https://bitbucket.org/lpresearch/openzen.git

- Create a build folder and run cmake:

.. code-block:: bash

    cd openzen
    mkdir build && cd build
    cmake ..

Now you can run the OpenZenExample:

.. code-block:: bash

    make -j4
    examples/OpenZenExample

OpenZen Build Options
=====================

These build options can be supplied to the cmake command to customize your OpenZen build. For example

.. code-block:: bash

    cmake -DZEN_BLUETOOTH=OFF -DZEN_PYTHON=ON ..


+------------------------+---------+---------------------------------------------------------------------------------+
| Name                   | Default | Description                                                                     |
+========================+=========+=================================================================================+
| ZEN_USE_STATIC_LIBS    | OFF     | Compile OpenZen as a static library                                             |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_STATIC_LINK_LIBCXX | OFF     | Option to statically link libstdc++ to be portable to older systems (Linux only)|
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_BLUETOOTH          | ON      | Compile with bluetooth support                                                  |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_BLUETOOTH_BLE      | OFF     | Compile with bluetooth low-energy support, needs Qt installed                   |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_NETWORK            | OFF     | Compile with support for network streaming of measurement data                  |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_CSHARP             | ON      | Compile C# bindings for OpenZen                                                 |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_PYTHON             | OFF     | Compile Python bindings for OpenZen                                             |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_TESTS              | ON      | Compile with OpenZen tests                                                      |
+------------------------+---------+---------------------------------------------------------------------------------+
| ZEN_EXAMPLES           | ON      | Compile with OpenZen examples                                                   |
+------------------------+---------+---------------------------------------------------------------------------------+
