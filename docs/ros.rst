############################
Robot Operating System (ROS)
############################

Overview
========
The OpenZen ROS driver is hosted in a separate `git repository <https://bitbucket.org/lpresearch/openzenros/>`_. In provides
the ROS package to readout OpenZen sensors and provide IMU and magnetometer measurements via ROS topics.

.. image:: images/ros-plot.png
   :alt: Visualization of angluar velocity data from an OpenZen sensor in rqt

Installing via the Package Manager
==================================

The OpenZen ROS driver is part of the official ROS distribution and you can conveniently install it via the package
manager of your Linux distribution. Please check this website to see if the OpenZen ROS driver is available
for the ROS distribution you use:

https://index.ros.org/p/openzen_sensor/bitbucket-lpresearch-openzenros/

For example, on Ubuntu 18.04 and with ROS distribution Melodic Morenia, the OpenZen ROS driver
can be installed with this command:

.. code-block:: bash

    apt install ros-melodic-openzen-sensor

Compilation
===========

If OpenZen is not available for your ROS distribution or you want to customize some OpenZen options,
you can also compile the OpenZen ROS driver yourself.

To compile the driver in your ROS setup, follow these steps:

.. code-block:: bash

    mkdir -p catkin_ws/src
    cd catkin_ws/src

    git clone --recurse-submodules https://bitbucket.org/lpresearch/openzenros.git

    # get your ROS environment going
    source /opt/ros/melodic/setup.bash
    cd ..
    catkin_make
    source ./devel/setup.bash

Running the Driver
==================

Before running the driver, please ensure that the access rights to the serial port on your
system are properly configured as described :ref:`in the Linux IO system section.<io-systems_linux-device>`

Open a new terminal window and run the ROS core:

.. code-block:: bash

    source /opt/ros/melodic/setup.bash
    roscore

You can then run the OpenZen ROS driver with this command in the window
you used to compile the software:

.. code-block:: bash

    rosrun openzen_sensor openzen_sensor_node

By default, it will connect to the first available sensor. If you want to connect to
a specific sensor, you can use the serial name of the sensor as parameter, for example:

.. code-block:: bash

    rosrun openzen_sensor openzen_sensor_node _sensor_name:="LPMSCU2000573"

If your sensor is configured for a different baud rate, you can use the baudrate parameter to
give a specfic baud rate setting:

.. code-block:: bash

    rosrun openzen_sensor openzen_sensor_node _sensor_name:="LPMSCU2000573" _baudrate:=115200

Now you can print the IMU values from ROS with:

.. code-block:: bash

    rostopic echo /imu/data

Or plot some values (for example linear acceleration) with 

.. code-block:: bash

    rosrun rqt_plot rqt_plot /imu/data/linear_acceleration

If you want to readout the values of two OpenZen sensors simultanously, you need to rename the topics and the node names likes this:

.. code-block:: bash

    rosrun openzen_sensor openzen_sensor_node __name:="cu2node" _sensor_name:="LPMSCU2000573" imu:=/cu2_imu
    rosrun openzen_sensor openzen_sensor_node __name:="ig1_node" _sensor_name:="LPMSIG1000032" imu:=/ig1_imu

You can also select another IO interface, for example Bluetooth:

.. code-block:: bash

    rosrun openzen_sensor openzen_sensor _sensor_interface:="Bluetooth" _sensor_name:="00:11:22:33:FF:EE"

Alternatively, you can use the sample launch file (openzen_lpms_ig1.launch) start data acquisition and plotting using openzen_sensor_node:

.. code-block:: bash

    roslaunch openzen_sensor openzen_lpms_ig1.launch

Limitations of OpenZen ROS driver
=================================

The default binary distribution and source compile for the OpenZen ROS driver does not support Bluetooth sensor models.
Therefore, if you want to use Bluetooth sensors together with ROS you need to follow the instructions above to compile the
OpenZen ROS driver. Before compiling the driver, you need to set the option ``ZEN_BLUETOOTH`` to ``ON`` in the CMakeLists.txt file in
the root folder of the OpenZenRos repository.

ROS API
=======

The openzen_sensor driver publishes orientation, angular velocity, linear acceleration
and magnetometer data (covariances are not yet supported). If your sensor models is equiped with
a GNSS receiver unit it also publishes the NavSatFix message.

Published Topics
################

/imu/data (`sensor_msgs/Imu <http://docs.ros.org/api/sensor_msgs/html/msg/Imu.html>`_) 
    Inertial data from the IMU. Includes calibrated acceleration, calibrated angular rates and orientation. The orientation is always unit quaternion. 

/imu/mag (`sensor_msgs/MagneticField <http://docs.ros.org/melodic/api/sensor_msgs/html/msg/MagneticField.html>`_)
    Magnetometer reading from the sensor.

/imu/nav (`sensor_msgs/NavSatFix <http://docs.ros.org/en/melodic/api/sensor_msgs/html/msg/NavSatFix.html>`_)
    Global position from a satellite navigation system. Only available if the IMU includes a GNSS chip.

/imu/is_autocalibration_active (`std_msgs/Bool <http://docs.ros.org/api/std_msgs/html/msg/Bool.html>`_)
    Latched topic indicating if the gyro autocalibration feature is active

Services
########

/imu/calibrate_gyroscope (`std_srvs/Empty <http://docs.ros.org/api/std_srvs/html/srv/Empty.html>`_) 
    This service activates the IMU internal gyro bias estimation function. Please make sure the IMU sensor is placed on a stable platform with minimal vibrations before calling the service. Please make sure the sensor is stationary for at least 4 seconds. The service call returns a success response once the calibration procedure is completed.

/imu/reset_heading (`std_srvs/Empty <http://docs.ros.org/api/std_srvs/html/srv/Empty.html>`_) 
    This service will reset the heading (yaw) angle of the sensor to zero. 

/imu/enable_gyro_autocalibration (`std_srvs/SetBool <http://docs.ros.org/melodic/api/std_srvs/html/srv/SetBool.html>`_)
    Turn on/off autocalibration function in the IMU. The status of autocalibration can be obtained by subscribing to the /imu/is_autocalibration_active topic. A message will published to /imu/is_autocalibration_active for each call to /imu/autocalibrate. 

Parameters
##########

~sensor_name (string, default: null) 
    By default, the library will connect to the first available sensor. If you want to connect to a specific sensor, you can use the serial name of the sensor as sensor_name parameter as follow:

    `$ rosrun openzen_sensor openzen_sensor_node _sensor_name:="LPMSCU2000573"`

~sensor_interface (string, default: LinuxDevice)
    Name of IO system for initiating sensor connection. Use "LinuxDevice" for usb connection in Linux. For more details, please check the documentation in the section :ref:`io-system-label`.

~baudrate (integer, default: 0)
    Baudrate in bits per seconds used to connect to the sensor. If the baudrate is left at 0, OpenZen will automatically pick the default baudrate for the respective sensor model.

~frame_id (string, default: imu) 
    The frame in which imu readings will be returned.
