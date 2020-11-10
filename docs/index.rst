`OpenZen <https://bitbucket.org/lpresearch/openzen>`_ is high performance sensor data streaming and processing.

Contents
--------

.. toctree::
    :maxdepth: 2

    ./setup
    ./getting_started
    ./examples
    ./supported_sensors
    ./data
    ./io_systems
    ./c_api
    ./cpp_api
    ./python_api
    ./csharp_api
    ./unity_plugin
    ./ros

Overview
--------

.. image:: https://lpresearch.bitbucket.io/openzen_resources/OpenZenUnityDemo.gif
   :alt: OpenZen Unity plugin connected to a LPMS-CU2 sensor and live visualization of sensor orientation

OpenZen Unity plugin connected to a LPMS-CU2 sensor and live visualization of sensor orientation.

OpenZen is a framework that allows access to Sensor Hardware from multiple vendors
without requiring understanding of the target hardware or communication layer by
providing a homogeneous API to application developers and a communication protocol
to hardware developers.

Currently, OpenZen has full support for communication over Bluetooth and USB (using
the Silabs driver), as well as experimental support for Bluetooth Low-Energy, CAN
(using the PCAN Basic driver) and USB (using the Ftdi driver). At the moment OpenZen
has only been used with IMU and GPS sensors, but is easily extensible to any type of sensor
or range of properties.

OpenZen can be used on Windows and Linux systems and provides a C, C++ and C# API.

.. TODO:
.. zeromq usage
.. describe Output value configuration in more detail
.. document more IO systems
