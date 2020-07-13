.. _examples-label:
###########################
Examples for OpenZen Usage
###########################

Connecting multiple Sensors
===========================

Its possible to connect multiple sensors with one OpenZen instance and event loops. Simply connect
to multiple sensors and store the sensor's handle:

.. code-block:: cpp

    auto sensorPairA = client.obtainSensorByName("SiUsb", "lpmscu2000574");
    auto& sensorA = sensorPairA.second;

    auto sensorPairB = client.obtainSensorByName("SiUsb", "lpmscu2000573");
    auto& sensorB = sensorPairB.second;

In your event loop, now check which sensor the last received event is orginating from:

.. code-block:: cpp

    auto event = client.waitForNextEvent();

    if (sensorA.sensor() == event.second.sensor) {
        std::cout << "Data from Sensor A" << std::endl;
    } else if (sensorB.sensor() == event.second.sensor) {
        std::cout << "Data from Sensor B" << std::endl;
    }
