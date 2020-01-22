#######
C++ API
#######

Overview
========
The OpenZen API is implemented using a pure CAPI to guarantee ABI stability. On top
of this we provide a modern C++ wrapper to simplify API interactions and allow RAII.
The C++ wrapper consists of three classes: ``zen::ZenClient``, ``zen::ZenSensor``,
and ``zen::ZenSensorComponent``. The client allows listing, obtaining and releasing of
sensors; as well as waiting for and polling events. The sensor - obtained through the
client - provides methods for executing, retrieving and updating properties of the
core sensor; in addition to updating firmware and IAP. Additionally, it offers
information about and access to the sensor components it houses. Each sensor component
in turn provides methods for executing, retrieving and updating the component's properties.
Examples of sensor components are an IMU, GPS and temperature sensor.

The C++ API is contained in the file ``OpenZen.h`` and has support for C++14 and C++17.
By default, the C++14 API will be used. If your project uses C++17 for compilation, the
C++17 API will be automatically used. You can also force the usage of the C++17 API by
defining the preprocessor variable ``OPENZEN_CXX17``.
The C++14 and C++17 APIs have the same method names but the C++17 uses more convenient
return types via the std::optional class.

A complete example of the C++ API usage can be found at this
`example source file <https://bitbucket.org/lpresearch/openzen/src/master/examples/main.cpp>`_.

Initialisation and Cleanup
==========================
In your application, you can create any numbers of clients to manage the sensors that
you want to communicate with. When a new client is created, the OpenZen backend needs
to connect to subsystems, which can result in failure. To initialize a client, call
the ``zen::make_client()`` function that returns an ``std::pair`` containing a potential ZenError
and ZenClient instance. Before you use the returned ZenClient class instance check
whether the returned error equals ``ZenError_None``.

.. code-block:: cpp

    std::pair<ZenError, ZenClient> make_client() noexcept

A ZenClient instance is automatically destructed when it goes out of scope. If you
want to terminate the client's event queue prior to this, call the ``ZenClient::close``
function. This could be the case in multi-threaded systems.

Events
======
Every ZenClient instance contains its own event queue which accumulates events
from all sensors that were obtained on that client. Events can either be polled
using ``ZenClient::pollNextEvent`` or waited for using ``ZenClient::waitForNextEvent``.
The only way to terminate a client that is waiting for an event, is by destroying
the client or preemptively calling ``ZenClient::close``.
