import sys
import openzen

error, client = openzen.make_client()
if not error == openzen.ZenError.NoError:
    print ("Error")
    sys.exit(1)

error, sensor = client.obtainSensorByName("LinuxDevice", "LPMSCU2000003", 0)

if not error == openzen.ZenSensorInitError.NoError:
    print ("Error connecting")
    sys.exit(1)


print ("Connected to sensor")

imu = sensor.getAnyComponentOfType("imu")
print(imu)
if imu is None:
    print ("No IMU found")
    sys.exit(1)

is_streaming = imu.getBoolProperty(openzen.ZenSensorPropertry.StreamData)
print ("is_streaming " + str(is_streaming))

runSome = 0
while True:
    zenEvent = client.waitForNextEvent()

    print(zenEvent.eventType)
    if (zenEvent.eventType == openzen.ZenEventType.ImuSample):
        print ("Got IMU data !")
        imu_data = zenEvent.data.imuData
        print(imu_data.a)
        print(type(imu_data.a))
        print ("A: {} m/s^2".format(imu_data.a))
        print ("G: {} degree/s".format(imu_data.g))

    runSome = runSome + 1
    if runSome > 400:
        break
