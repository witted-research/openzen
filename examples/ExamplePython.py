###########################################################################
#
# OpenZen Python example
#
# Make sure the _OpenZen.pyd (for Windows) or OpenZen.so (Linux/Mac)
# and the OpenZen.py file are in the same folder as this file.
# If you want to connect to USB sensors on Windows, the file SiUSBXp.dll
# should also be in the same folder
#
###########################################################################

from OpenZen import *

# name of the IO interfae the Sensor is connected to
# see https://lpresearch.bitbucket.io/openzen/latest/io_systems.html for options
sensor_io = "SiUsb"
# name of the sensor
sensor_name = "lpmscu2000573"

ZenSetLogLevel(ZenLogLevel_Info)
client_handle = ZenClientHandle_t()
res_init = ZenInit(client_handle)
print("Initialized successful {}".format(res_init == ZenError_None))

if not res_init == ZenError_None:
    exit(1)

print("Connecting to {}".format(sensor_name))

sensor_handle = ZenSensorHandle_t()
res_obtain = ZenObtainSensorByName(client_handle, sensor_io, sensor_name, 0, sensor_handle)

print("Obtaining sensor successful {}".format(res_obtain == ZenSensorInitError_None))

if not res_obtain == ZenSensorInitError_None:
    exit(1)

# get the first IMU component of the sensor
imu_component_handle = ZenComponentHandle_t()
ZenSensorComponentsByNumber(client_handle, sensor_handle, g_zenSensorType_Imu, 0, imu_component_handle)

if imu_component_handle.handle == 0:
    print("Connected sensor does not have an IMU component")
    exit(1)

# set a sensor property
# enable output of Orientation of the Sensor in Quaternion
ZenSensorComponentSetBoolProperty(client_handle, sensor_handle, imu_component_handle,
    ZenImuProperty_OutputQuat, True)

## get some events
for poll_i in range(200):
    zen_event = ZenEvent()
    ZenWaitForNextEvent(client_handle, zen_event)
    if zen_event.component.handle == imu_component_handle.handle:
        # output the sensor orientation
        q_python = OpenZenFloatArray_frompointer(zen_event.data.imuData.q)
        print ("IMU Orientation - w: {} x: {} y: {} z: {}"
            .format(q_python[0], q_python[1], q_python[2], q_python[3]))

ZenReleaseSensor(client_handle, sensor_handle)
ZenShutdown(client_handle)

print ("Sensor connection closed")