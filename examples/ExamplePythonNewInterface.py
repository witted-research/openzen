import sys
import openzen

openzen.set_log_level(openzen.ZenLogLevel.Warning)

error, client = openzen.make_client()
if not error == openzen.ZenError.NoError:
    print ("Error")
    sys.exit(1)

#error, sensor = client.obtainSensorByName("LinuxDevice", "LPMSCU2000003", 0)

error = client.list_sensors_async()

# check for events
sensor_desc_connect = None
while True:
    zenEvent = client.wait_for_next_event()

    if zenEvent.event_type == openzen.ZenEventType.SensorFound:
        print ("Found sensor {} on IoType {}".format( zenEvent.data.sensor_found.name,
            zenEvent.data.sensor_found.io_type))
        sensor_desc_connect = zenEvent.data.sensor_found

    if zenEvent.event_type == openzen.ZenEventType.SensorListingProgress:
        print (zenEvent.data.sensor_listing_progress)
        if zenEvent.data.sensor_listing_progress.complete > 0:
            print ("Sensor listing complete")
            break
print ("all sensors listed")

if sensor_desc_connect is None:
    print("No sensor found")
    sys.exit(1)

error, sensor = client.obtain_sensor(sensor_desc_connect)

if not error == openzen.ZenSensorInitError.NoError:
    print ("Error connecting")
    sys.exit(1)


print ("Connected to sensor")

imu = sensor.getAnyComponentOfType("imu")

print("type is " + imu.type)
#print("sensor is " + imu.sensor)

print(imu)
if imu is None:
    print ("No IMU found")
    sys.exit(1)

is_streaming = imu.getBoolProperty(openzen.ZenImuProperty.StreamData)
print ("is_streaming " + str(is_streaming))

accAlign = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]

errSetting = imu.setArrayPropertyFloat(openzen.ZenImuProperty.AccAlignment, accAlign)
print("Stored alignement", errSetting)

#print(type(imu.getArrayProperty(openzen.ZenImuProperty.AccAlignment)))
#print(imu.getArrayProperty(openzen.ZenImuProperty.AccAlignment))
arrErr, accAlignment = imu.getArrayPropertyFloat(openzen.ZenImuProperty.AccAlignment)

print ("Alignment: {}", accAlignment)

#sys.exit(0)

runSome = 0
while True:
    zenEvent = client.wait_for_next_event()

    print(zenEvent.event_type)
    if (zenEvent.event_type == openzen.ZenEventType.ImuSample):
        print ("Got IMU data !")
        imu_data = zenEvent.data.imu_data
        print(imu_data.a)
        print(type(imu_data.a))
        print ("A: {} m/s^2".format(imu_data.a))
        print ("G: {} degree/s".format(imu_data.g))

    runSome = runSome + 1
    if runSome > 400:
        break
print("Copying sensor")
anotherSensor = sensor
print("releasing")
anotherSensor.release()
print("release done")

print ("releasing client")
client.close()
print("done")

print("exiting")