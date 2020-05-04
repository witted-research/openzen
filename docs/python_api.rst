##########
Python API
##########

Overview
========
The OpenZen language bindings allow you to access sensor data from Python.
The function names are the same which are used by the OpenZen C API.

You can find a complete OpenZen Python example in this `file <https://bitbucket.org/lpresearch/openzen/src/master/examples/ExamplePython.py>`_.

Building OpenZen with Python support
====================================

The binary release and default build of OpenZen does not include Python support.
To compile it for the exact Python version used on your system, enable to build
the Python support with this CMake option:

.. code-block:: bash

    cmake -DZEN_PYTHON=On ..

This will search for a valid Python3 installation on your system and build the
OpenZen binaries to support this Python version.

The output folder will now contain the file ``OpenZen.py`` and the files 
``_OpenZen.so`` (on Linux/Mac) and ``_OpenZen.pyd`` (on Windows).

Place these files into the same folder you run your Python script from and the
OpenZen module can be imported.

Initialize OpenZen in Python
============================

To create a new instance of OpenZen, you can use this code snippet:

.. code-block:: python

    from OpenZen import *

    client_handle = ZenClientHandle_t()
    res_init = ZenInit(client_handle)

Receive Sensor Data in Python
=============================

OpenZen events containing sensor data need to be read from the pointers returned
by the interface using the following method:

.. code-block:: python

    zen_event = ZenEvent()
    ZenWaitForNextEvent(client_handle, zen_event)
    if zen_event.component.handle == imu_component_handle.handle:
        # output the sensor orientation
        q_python = OpenZenFloatArray_frompointer(zen_event.data.imuData.q)
        print ("IMU Orientation - w: {} x: {} y: {} z: {}"
            .format(q_python[0], q_python[1], q_python[2], q_python[3]))
