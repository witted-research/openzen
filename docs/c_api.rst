#####
C API
#####

Overview
========
The OpenZen API is implemented using a pure C API to guarantee ABI stability. The header
file ``OpenZenCAPI.h`` can be included to access the C interface to OpenZen.

The overall concept of OpenZen usage is described in the section :ref:`getting-started-label`.
This section will describe some special considerations when using the C API. A complete
example of the C API usage can be found at this 
`example source file <https://bitbucket.org/lpresearch/openzen/src/master/examples/ExampleCAPI.c>`_.

Initialisation and Cleanup
==========================
In your application, you can create any numbers of clients to manage the sensors that
you want to communicate with. When a new client is created, the OpenZen backend needs
to connect to subsystems, which can result in failure. To initialize a client, call
the ``ZenInit`` function with a pointer to the ``ZenClientHandle_t`` struct. This struct
will then hold the handle to communicate with OpenZen on future calls.
Before you use this handle, check whether the returned error equals ``ZenError_None``.

.. code-block:: c

    ZenError_t zenError = 0;
    ZenClientHandle_t clientHandle = { 0 };
    zenError = ZenInit(&clientHandle);
    if (zenError != ZenError_None) {
        printf("ZenError %d when obtaining client.\n", zenError);
        return -1;
    }

To release all resources associated with the OpenZen client, you can call the ``ZenShutdown``
function:

.. code-block:: c

    ZenShutdown(clientHandle);

Events
======
Every ZenClientHandle_t instance contains its own event queue which accumulates events
from all sensors that were obtained on that client. Events can either be polled
using ``ZenPollNextEvent`` or waited for using ``ZenWaitForNextEvent``.
The only way to terminate a client that is waiting for an event, is by destroying
the client or preemptively calling ``ZenReleaseSensor``.

Access to Sensors and Components
================================
To query the available sensors and connect them can be done using the functions ``ZenListSensorsAsync``,
``ZenObtainSensor``, ``ZenObtainSensorByName``. The usage of these functions is described
in the section :ref:`getting-started-label`.

Once a sensor handle has been obtained, it needs to be provided to all ``ZenSensor*`` functions as a parameter.

A sensor component can be retrieved with the function call ``ZenSensorComponents`` by providing the
type of sensor component which should be loaded. Currently, the component types ``g_zenSensorType_Imu``
and ``g_zenSensorType_Gnss`` are supported.
