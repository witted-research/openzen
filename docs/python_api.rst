##########
Python API
##########

Overview
========
The OpenZen language bindings allow you to access sensor data from Python.

You can find a complete OpenZen Python example in this `file <https://bitbucket.org/lpresearch/openzen/src/master/examples/ExamplePython.py>`_.

Using Python with OpenZen Releases
==================================

The binary releases of OpenZen for Windows and Linux include support for **Python 3.8 64-bit** .
On Linux, the library file ``openzen.so`` can be directly imported by Python. On Windows this file
is named ``openzen.pyd``. You can copy this library to the location where you execute your python scripts.

The other option is to set the ``PYTHONPATH`` environment variable to the folder where ``openzen.so``
is located so the OpenZen files can be found when you call ``import openzen``.

The last option is to set the PYTHONPATH dynamically when starting your script and before you
call ``import openzen``.

.. code-block:: python

    sys.path.append("C:/OpenZenRelease/")
    import openzen

Building OpenZen with Python support
====================================

The binary release of OpenZen includes support for Python 3.8. If you want to use a
different Python version, you can compile OpenZen for that specific version.
To compile it for the exact Python version used on your system, enable to build
the Python support with this CMake option:

.. code-block:: bash

    cmake -DZEN_PYTHON=ON ..

This will search for a valid Python3 installation on your system and build the
OpenZen binaries to support this Python version.

The output folder will now contain the file ``openzen.so`` (on Linux/Mac) and ``openzen.pyd`` (on Windows).

Place these files into the same folder you run your Python script from and the
OpenZen module can be imported.

Initialize OpenZen in Python
============================

To create a new instance of OpenZen, you can use this code snippet:

.. code-block:: python

    import openzen

    openzen.set_log_level(openzen.ZenLogLevel.Warning)

    error, client = openzen.make_client()
    if not error == openzen.ZenError.NoError:
        print ("Error while initializinng OpenZen library")
        sys.exit(1)

Receive Sensor Data in Python
=============================

OpenZen events containing sensor data need to be read from the pointers returned
by the interface using the following method:

.. code-block:: python

    zenEvent = client.wait_for_next_event()

    # check if its an IMU sample event and if it
    # comes from our IMU and sensor component
    if zenEvent.event_type == openzen.ZenEventType.ImuData and \
        zenEvent.sensor == imu.sensor and \
        zenEvent.component.handle == imu.component.handle:

        imu_data = zenEvent.data.imu_data
        print ("A: {} m/s^2".format(imu_data.a))
        print ("G: {} degree/s".format(imu_data.g))

Troubleshooting
===============

32-bit issues
-------------

If you get the following error when importing OpenZen:

.. code-block:: bash

    ImportError: DLL load failed while importing openzen: %1 is not a valid Win32 application.

the reason is most probably that you tried to load OpenZen with a 32-bit Python version. The binary
Release of OpenZen only supports 64-bit versions of Python. Please make sure you have the 64-bit version
of Python installed. The 64-bit version can be selected on the Python `download page <https://www.python.org/downloads/windows/>`_
under the name ``Windows x64-68``.

PYTHONPATH not properly set up
------------------------------

If you get an error message of this form:

.. code-block:: bash

    ModuleNotFoundError: No module named 'openzen'

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

    ImportError: dynamic module does not define init function (initopenzen)

then OpenZen was compiled with Python 3 and you are trying to use with with Python 2. Make sure you
call the OpenZen script with Python3:

.. code-block:: bash

    python3 ExamplePython.py
