# Changelog

Changes and additions to OpenZen will be documented in this file.

## Version 1.2 - 2020/11/11

- This release breaks the ABI compatibitly with previous OpenZen releases. If you want to use this version of
  OpenZen you have to recompile old projects with the C/C++ headers of this version.
- ZenEventType replaces the ZenSensorEvent, ZenImuEvent and ZenGnssEvent enums. Now the field ZenEvent::eventType
  directly uses the ZenEventType and a comparison can be perfromed to check which type of event is provided:
```
  if (event.eventType == ZenEventType_ImuData) {
    // access event.data.imuData.g for gyro data
  }
```
- ZenHeaveMotionData struct in ZenImuData has been replaced by the member heaveMotion.
- Gyroscope sensor data in ZenImuData.g and ZenImuData.gRaw will now be always in degrees/s and
  the euler angles in ZenImuData.r will always be in degrees. Even if the hardware sensor (feature in IG1 firmware)
  is set to output in radians, OpenZen will ensure the values are converted to degrees/s and degrees when
  ouputting the values via the OpenZen API.
- added frameCount field to the ZenGnssData structure
- changed python bindings to a more convenient wrapper code
- updated documentation with supported sensor models and explanation on the sensor output values
- Properly handling a possible Bluetooth exception
- Improved stability on initial sensor connection

## Version 1.1.3 - 2020/09/03

- added support for IG1 RS485
- fixed bug with shutting down serial communication on Linux/Mac
- refactored sensor data parsing code
- added CMake unity build option for faster builds
- added feature to compore sensor handles in C++ API
- added support for 800 Hz IMU firmware
- fixed bug in computation of timestamp
- fixed typo in CAN Bus baudrate assignment
- fixed typo in CAN Bus baudrate assignment
- move binary and proprierary libraries to independent repository
  and made the download optional

## Version 1.1.2 - 2020/06/04

- turn static linking on for ARM build

## Version 1.1.1 - 2020/06/04

- Added Python 3.8 for Windows release
- Improved documentation for Python support
- Added Python example script to the release build which works out-of-the box

## Version 1.1.0 - 2020/05/04

- Added C# and C++ example to the Window release
- Removed Qt Bluetooth and accessing Bluetooth API directly on Win/Linux/Mac
- Added Python API support and example
- Added support for MacOS
- Improved Copyright documentation
- Improved documentation

## Version 1.0.0 - 2020/02/10

- Initial Release of OpenZen