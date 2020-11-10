.. _getting-started-label:

############################
Getting Started with OpenZen
############################

Overview
========
OpenZen allows to access many different sensor models which are connected via different transports layers with one
unified programming interface. This allows to develop applications which support a wide range of sensors and allows
to easily adapt your application to new or additional sensors.

The following section will walk you through the process of accessing, configuring and streaming data from
a sensor connected to OpenZen. If you prefer to study and run a full code example, please have a look
at this `example source file <https://bitbucket.org/lpresearch/openzen/src/master/examples/main.cpp>`_.

IO Systems
----------
To communicate with the sensor hardware, OpenZen provides multiple so-called **IO systems**. Each of these can establish
the communication to sensors and can list the available sensor hardware. Here are some examples of IO systems available
in OpenZen:

- USB
- Bluetooth
- Network streaming

You can find the full list of available IO systems in the section :ref:`io-system-label`.

Sensor Components
-----------------
A sensor can have multiple components which supply various types of sensor data. Once the connection
to a sensor has been established, the available components can be queried and their properties can
be access and modified. Furthermore, the streaming of measurement data can be started for each component.

At this time OpenZen supports the following components:

**Inertial Measurement Unit (IMU)**

Provides accelerometer and gyroscope measurements and the orientation result
from the sensor fusion running on the connected sensor.

**Global Navigation Satellite System (GNSS)**

Provides global position measurements via GPS, BeiDou, GLONASS and Galileo, the associated
error estimates and information on the quality and mode of the GNSS fix.

Creating an OpenZen Client
==========================
The following description will use the C++ interface to OpenZen but other language bindings use the same concepts
described here. To see the description how to use the other language bindings, please have a look at the respective
section for the language you are interested in.

In order to acquire sensor access via OpenZen, you first need to create an OpenZen client. This can be done with the
utility method ``zen::make_client``. This call returns an instance of the ZenClient class:

.. code-block:: cpp

    auto clientPair = zen::make_client();
    auto& clientError = clientPair.first;
    auto& client = clientPair.second;

    if (clientError)
        // error when creating OpenZen client
        return clientError;

Receiving Events from OpenZen
=============================
Every ZenClient instance contains its own event queue which accumulates events
from all sensors that were obtained on that client. Events can either be polled
using ``ZenClient::pollNextEvent`` or waited for using ``ZenClient::waitForNextEvent``.
The only way to terminate a client that is waiting for an event, is by destroying
the client or preemptively calling ``ZenClient::close``.

.. this maybe explain how to setup an event loop here ?

Listing Available Sensors
=========================
The IO systems available on your platform can automatically list all available
sensors connected to the system. In order to do this, you need to call the method
``ZenClient::listSensorsAsync`` to start the query for available sensors. Depending
on the IO systems, it can take a couple of seconds for the listing to be complete.

.. code-block:: cpp

    if (auto error = client.listSensorsAsync())
    {
        // error while listing available senors
        return error;
    }

The ``ZenClient::listSensorsAsync`` method will return immediately and
the information on the found sensors will be send to ZenClient's event queue and
can be retrieved with calls to ``ZenClient::pollNextEvent`` or
``ZenClient::waitForNextEvent``. You can either do this on your applications main
thread or use a background thread to retrieve the event listing data.

The following code snippets shows an example how to receive the OpenZen events
which contain the information during the sensor discovery process.

If the ``event.component.handle`` is 0, the event is not specific to a sensor
but an event which is related by the OpenZen client itself, like the sensor
discovery events. Next, we can check the ``event.eventType`` field for the
``ZenSensorEvent_SensorFound`` and ``ZenSensorEvent_SensorListingProgress``
event types. For the ``ZenSensorEvent_SensorFound`` event, the field
``event.data.sensorFound`` is of type ``ZenSensorDesc`` which contains the
sensor's name, serial number and all information required to connect to the sensor.

.. code-block:: cpp

    bool listingComplete = false;
    while (!listingComplete) {
        const auto pair = client.waitForNextEvent();
        const bool success = pair.first;
        auto& event = pair.second;
        if (!success)
            break;

        if (!event.component.handle)
        {
            switch (event.eventType)
            {
            case ZenEventType_SensorFound:
                std::cout << "Found sensor " << event.data.sensorFound.name << std::endl;
                break;

            case ZenEventType_SensorListingProgress:
                std::cout << "Sensor listing progress " << event.data.sensorListingProgress.progress
                    << " %" << std::endl;
                if (event.data.sensorListingProgress.complete) {
                    listingComplete = true;
                }
                break;
            }
        }
    }

Connecting to a Sensor
======================
A sensor found by the ``ZenClient::listSensorsAsync`` call can be connected via the OpenZen
client using the ``ZenSensorDesc`` which is contained in the ``event.data.sensorFound`` of
the ``ZenSensorEvent_SensorFound``. The method call to ``ZenClient::obtainSensor`` returns a
``std::pair`` where the first entry is a possible error code and the second entry is an
instance of the object ``zen::ZenSensor`` which can be used to access the Sensor's
properties.

.. code-block:: cpp

    auto sensorPair = client.obtainSensor(sensorDesc);
    auto& obtainError = sensorPair.first;
    auto& sensor = sensorPair.second;
    if (obtainError)
    {
        // error while obtaining the sensor
        return obtainError;
    }

Sensors can also connected directly if the IO system they are connected too and their
name is known already. Here, the method ``ZenClient::obtainSensorByName`` can be called
with the name of the IO system and the name of the sensor:

.. code-block:: cpp

    // connect the sensor with the name lpmscu2000573 via the SiLabs USB IO System
    auto sensorPair = client.obtainSensorByName("SiUsb", "lpmscu2000573");
    auto& obtainError = sensorPair.first;
    auto& sensor = sensorPair.second;
    if (obtainError)
    {
        // error while obtaining the sensor
        return obtainError;
    }

Please check the documentation in the section :ref:`io-system-label`. for the available
IO systems and which naming conventions they use to identify connected sensors.

You can connect multiple sensor via one ``ZenClient`` and the events of all
sensor will be available on the event queue of the ``ZenClient`` instance.

Reading and Modifying Sensor Properties
=======================================
OpenZen allows to read an modify the properties of connected sensors. Which properties
are available depends on the concrete sensor connected. You can find more information
on sensor properties in the section ref:`supported-sensors-label`.

Each sensor property in OpenZen has a specific data type and the respective method needs
to be used on the ``zen::ZenSensor`` instance.

.. code-block:: cpp

    auto sensorModelPair = sensor.getStringProperty(ZenSensorProperty_SensorModel);
    auto & sensorModelError = sensorModelPair.first;
    auto & sensorModelName = sensorModelPair.second;
    if (sensorModelError) {
        // error while reading the string property from the sensor
        return sensorModelError;
    }
    std::cout << "Sensor Model: " << sensorModelName << std::endl;


Accessing Sensor Components
===========================
To access a specific component of a sensor, the method call ``ZenSensor::getAnyComponentOfType``
can be used to retrieve the component of a specific type. If this component is not available
on this sensor, an error will be returned.

.. code-block:: cpp

    auto imuPair = sensor.getAnyComponentOfType(g_zenSensorType_Imu);
    auto& hasImu = imuPair.first;
    auto imu = imuPair.second;

    if (!hasImu)
    {
        // error, this sensor does not have an IMU component
        return ZenError_WrongSensorType;
    }

As with the ``ZenSensor`` class the ``ZenSensorComponent`` returned by ``ZenSensor::getAnyComponentOfType``
call can be used to access and modify the properties of the sensor component.

.. set output values
.. set streaming freq

Reading Sensor Values
=====================

To start receiving data from a connected sensor, you need to ensure that the sensor
is in streaming mode to send out data on its own:

.. code-block:: cpp

    if (auto error = imu.setBoolProperty(ZenImuProperty_StreamData, true))
    {
        // cannot set sensor into streaming mode
        return error;
    }

Now, sensor events with measurement data will be available on the event queue of the OpenZen client.
You can use the previously introduced methods ``ZenClient::pollNextEvent`` or
``ZenClient::waitForNextEvent`` to retrieve the sensor data of the inertial measurement unit:

.. code-block:: cpp

    const auto pair = client.waitForNextEvent();
    const bool success = pair.first;
    auto& event = pair.second;
    if (!success)
        break;

    // ensure the event is from the IMU component
    if (event.component.handle == imu.component().handle)
    {
        switch (event.eventType)
        {
        case ZenEventType_ImuData:
                std::cout << "> Acceleration: \t x = " << event.data.imuData.a[0]
                    << "\t y = " << event.data.imuData.a[1]
                    << "\t z = " << event.data.imuData.a[2] << std::endl;
                std::cout << "> Gyro: \t\t x = " << event.data.imuData.g[0]
                    << "\t y = " << event.data.imuData.g[1]
                    << "\t z = " << event.data.imuData.g[2] << std::endl;
            break;
        }
    }

To process the GNSS data streamed from the sensor, you can filter for events coming from
the GNSS component like this:

.. code-block:: cpp

    if (event.component.handle == g_gnssHandle)
    {
        switch (event.eventType)
        {
        case ZenEventType_GnssData:
                std::cout << "> GPS Fix Type: \t = " << event.data.gnssData.fixType << std::endl;
                std::cout << "> Longitude: \t = " << event.data.gnssData.longitude
                    << "   Latitude: \t = " << event.data.gnssData.latitude << std::endl;
            break;
        }
    }

.. todo: check for the correct sensor id

Closing the Sensor Connection
=============================
Once you are done with sampling sensor values, you can release the connection to the
sensor and close the connection with the client:

.. code-block:: cpp

    sensor.release();
    client.close();
