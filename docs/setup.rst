#####
Setup
#####

Dowloading pre-compiled OpenZen
===============================

The pre-compiled binaries for OpenZen on Windows are available to download here:

`OpenZen Releases <https://bitbucket.org/lpresearch/openzen/downloads/>`_

The folder example in this download contains a full example how to use OpenZen
together with VisualStudio 2017 on Windows 64-bit.

Building OpenZen from Source
============================

OpenZen uses CMake (3.11 or higher) as build system, and has been built
and tested on Windows (MSVC) and Ubuntu (gcc7). As OpenZen is written in
C++17, an up-to-date compiler is required. The following steps guide you
through building OpenZen for Windows and Linux.

Windows
-------

- Install MSVC build tools, or the Visual Studio IDE (requires C++17 support)
- Install CMake, or a GUI (e.g. Visual Studio) that incorporates CMake
- Install Qt (5.11.2 or higher)
- Clone the external repositories:
.. code-block:: bash

    git submodule update --init

- Configure CMake with the environment variable CMAKE_PREFIX_PATH pointing towards your Qt bin directory

You can do that by inserting this line (with the correct Qt installation path on your system)

.. code-block:: cmake

   set(CMAKE_PREFIX_PATH "C://Qt//5.12.3//msvc2017_64//")

in the topmost CMakeLists.txt
Now you can run the OpenZenExample using MSVC.

Linux
-----

- Install gcc7 (requires C++17 support): sudo apt-get install gcc-7
- Install CMake (instructions)
- Install Qt (5.11.2 or higher): sudo apt-get install qtbase5-dev qtconnectivity5-dev
- Clone the external repositories:
.. code-block:: bash

    git submodule update --init

- Create a build folder and run cmake:
.. code-block:: bash

    mkdir build && cd build
    cmake ..

Now you can run the OpenZenExample:

.. code-block:: bash

    make -j4
    examples/OpenZenExample
