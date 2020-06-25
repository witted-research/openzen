##########
Python API
##########

Overview
========
The OpenZen language bindings allow you to access sensor data from Python.
The function names are the same which are used by the OpenZen C API.

You can find a complete OpenZen Python example in this `file <https://bitbucket.org/lpresearch/openzen/src/master/examples/ExamplePython.py>`_.

Using Python with OpenZen Releases
==================================

The binary releases of OpenZen for Windows and Linux include support for **Python 3.8 64-bit** .
You can find all files needed in the folder ``bindings\OpenZenPython``. In order to
import the OpenZen module in your project, you can copy all the files
in ``bindings\OpenZenPython`` into the folder of your Python script.

The other option is to set the ``PYTHONPATH`` environment variable to the ``bindings\OpenZenPython``
folder so your python installation can find the OpenZen files when you call ``from OpenZen import *``.

The last option is the set the PYTHONPATH dynamically when starting your script and before you
call ``from OpenZen import *``. This is what is done in the ``ExamplePython.py`` script provided
with the OpenZen release:

.. code-block:: python

    sys.path.append("C:/OpenZenRelease/bindings/OpenZenPython")
    from OpenZen import *

Building OpenZen with Python support
====================================

The binary release of OpenZen includes support for Python 3.8. If you want to use a
different Python version, you can compile OpenZen for that specific version.
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

Troubleshooting
===============

32-bit issues
-------------

If you get the following error when importing OpenZen:

.. code-block:: bash

    ImportError: DLL load failed while importing _OpenZen: %1 is not a valid Win32 application.

the reason is most probably that you tried to load OpenZen with a 32-bit Python version. The binary
Release of OpenZen only supports 64-bit versions of Python. Please make sure you have the 64-bit version
of Python installed. The 64-bit version can be selected on the Python `download page <https://www.python.org/downloads/windows/>`_
under the name ``Windows x64-68``.

PYTHONPATH not properly set up
------------------------------

If you get an error message of this form:

.. code-block:: bash

    ModuleNotFoundError: No module named 'OpenZen'

the PYTHONPATH for Python to find the OpenZen files is not properly set up. Please follow the instructions above
to setup the PYTHONPATH.

Conficting Python version I
---------------------------

If you get an error message of this form:

.. code-block:: bash

    ImportError: Module use of python38.dll conflicts with this version of Python.

or

.. code-block:: bash

    ImportError: DLL load failed: The specified module could not be found.

the Pyton version you intend to use is not supported by the OpenZen binary release. Only one Python
version is supported py the binary release of OpenZen. Can can either switch to Python version 3.8 64-bit or compile
OpenZen with support for the Python version you intent to use. Please see the section above on how to
compile Python with support for your version.

Conficting Python version II
----------------------------

If you get an error message of this form:

.. code-block:: bash

    ImportError: dynamic module does not define init function (init_OpenZen)

then OpenZen was compiled with Python 3 and you are trying to use with with Python 2. Make sure you
call the OpenZen script with Python3:

.. code-block:: bash

    python3 ExamplePython.py
