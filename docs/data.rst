####################################
Sensor Configuration and Data Output
####################################

Overview
========
This section gives you an overview of the configuration options and data output of your sensor. For more detailed
and sensor-specfic explanations please see the documentation for the respective sensors. Furthermore, a detailed description
of the orientation sensor fusion and how to optimize it for your application case can be found there.

Data Output Settings
====================
Will all sensors models, the data which is transmitted can be selected individually to save bandwith and improve the
latency of the data transfer. Therefore, you need to ensure the data item you want to read out is enabled for output.
There are two ways to enable or disable the output of a data item.

**1. Use the graphical user interface Tools LPMS-Control and IG1-Control**

You can download theses tools from our `support website <https://lp-research.com/support/>`_. Then connect to the sensor
and enable the output of the data you require. After enabling the output please ensure the settings are persisent by writing
them to the sensor flash.

Here is an example where to change the sensor data output:

.. image:: images/lpms-sensor_output.png
   :alt: LpmsControl enable data ouput

**2. Configure data output with OpenZen**

You can also enable or disable the output data set with OpenZen after you have opened a connection to the sensor:

.. code-block:: cpp

    imu.setBoolProperty(ZenImuProperty_OutputEuler, false);
    imu.setBoolProperty(ZenImuProperty_OutputRawGyr, true);
    ...

ZenImuData
==========
The ZenImuData structure contains the measurements of accelerometer and gyroscope sensors. Data items which are not
enabled for output or not supported by the sensor are kept at their default values.

+------------+------------------+------------------------------------+
| Field Name | Unit             | Description                        |
+============+==================+====================================+
| frameCount | no unit          | Data frame number assigned to this |
|            |                  | measurement by the firmware.       |
+------------+------------------+------------------------------------+
| timestamp  | s                | Timestamp of the measurement data. |
|            |                  | The start point of this timestamp  |
|            |                  | is arbitrary but subsequent        |
|            |                  | measurements are guaranteed to have|
|            |                  | the distance to each other in time.|
+------------+------------------+------------------------------------+
| a          | m/s^2            | Accleration measurment after all   |
|            |                  | corrections have been applied      |
+------------+------------------+------------------------------------+
| g          | rad/s or         | Gyroscope measurment after all     |
|            | deg/s            | corrections have been applied      |
+------------+------------------+------------------------------------+
| b          | :math:`\mu T`    | Magnetometer measurment after all  |
|            |                  | corrections have been applied      |
+------------+------------------+------------------------------------+
| aRaw       | m/s^2            | Accleration measurment before all  |
|            |                  | corrections have been applied      |
+------------+------------------+------------------------------------+
| gRaw       | rad/s or         | Gyroscope measurment before all    |
|            | deg/s            | corrections have been applied      |
+------------+------------------+------------------------------------+
| w          | rad/s or         | Angular veloctiy                   |
|            | deg/s            |                                    |
+------------+------------------+------------------------------------+
| r          | rad/s or         | Three euler angles representing    |
|            | deg/s            | the current rotation of the sensor.|
|            |                  | See the sensor documenation how    |
|            |                  | the angles are defined             |
+------------+------------------+------------------------------------+
| q          | no unit          | Quaternion representing the current|
|            |                  | rotation of the sensor in this     |
|            |                  | order: w, x, y,z                   |
|            |                  | See the sensor documenation how the|
|            |                  | rotation axes are defined.         |
+------------+------------------+------------------------------------+
| rotationM  | no unit          | Orientation data as rotation matrix|
|            |                  | without offset applied.            |
+------------+------------------+------------------------------------+
| rotOffsetM | no unit          | Orientation data as rotation matrix|
|            |                  | with offset applied.               |
+------------+------------------+------------------------------------+
| pressure   | mPa              | Barometric pressure measurement.   |
|            |                  | Not supported by all sensor models.|
+------------+------------------+------------------------------------+
| heaveMotion| m                | Heave motion output.               |
|            |                  | Not supported by all sensor models.|
+------------+------------------+------------------------------------+