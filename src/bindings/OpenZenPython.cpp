#include "OpenZen.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <array>

namespace py = pybind11;

using namespace zen;

/*
On Windows, the openzen.dll file needs to be renamed to
openzen.pyd in order for Python to import it!
On Linux it needs to be named openzen
*/

namespace OpenZenPythonHelper {
    template<typename TDataType, int TSize, typename TFrom>
    inline std::array<TDataType, TSize> toStlArray(TFrom const& from) {
        std::array<TDataType, TSize> stl_array;
        std::copy_n(std::begin(from), TSize, stl_array.begin());
        return stl_array;
    }
};

PYBIND11_MODULE(openzen, m) {

    // C part of the data types from ZenTypes.h
    py::enum_<ZenError>(m, "ZenError")
        .value("NoError", ZenError_None)
        .value("IsNull", ZenError_IsNull)
        .value("NotNull", ZenError_NotNull);

    py::enum_<EZenImuProperty>(m, "ZenSensorPropertry")
        .value("Invalid", ZenImuProperty_Invalid)
        .value("StreamData", ZenImuProperty_StreamData);

    py::enum_<ZenSensorInitError>(m, "ZenSensorInitError")
        .value("NoError", ZenSensorInitError_None)
        .value("InvalidHandle", ZenSensorInitError_InvalidHandle);

    py::enum_<ZenEventType>(m, "ZenEventType")
        .value("NoType", ZenEventType_None)
        .value("SensorFound", ZenEventType_SensorFound)
        .value("SensorListingProgress", ZenEventType_SensorListingProgress)
        .value("SensorDisconnected", ZenEventType_SensorDisconnected)
        .value("ImuSample", ZenEventType_ImuSample)
        .value("GnssSample", ZenEventType_GnssSample);

/*
typedef union
{
    ZenEventData_Imu imuData;
    ZenEventData_Gnss gnssData;
    ZenEventData_SensorDisconnected sensorDisconnected;
    ZenEventData_SensorFound sensorFound;
    ZenEventData_SensorListingProgress sensorListingProgress;
} ZenEventData;
*/
    py::class_<ZenEventData>(m, "ZenEventData")
        .def_readonly("imuData", &ZenEventData::imuData);

/* 
typedef struct ZenImuData
{
    /// Calibrated accelerometer sensor data.
    float a[3];

    /// Calibrated gyroscope sensor data.
    float g[3];

    /// Calibrated magnetometer sensor data.
    float b[3];

    /// Raw accelerometer sensor data.
    float aRaw[3];

    /// Raw gyroscope sensor data.
    float gRaw[3];

    /// Raw magnetometer sensor data.
    float bRaw[3];

    /// Angular velocity data.
    float w[3];

    /// Euler angle data.
    float r[3];

    /// Quaternion orientation data.
    /// The component order is w, x, y, z
    float q[4];

    /// Orientation data as rotation matrix without offset.
    float rotationM[9];

    /// Orientation data as rotation matrix after zeroing.
    float rotOffsetM[9];

    /// Barometric pressure.
    float pressure;

    /// Index of the data frame.
    int frameCount;

    /// Linear acceleration x, y and z.
    float linAcc[3];

    /// Gyroscope temperature.
    float gTemp;

    /// Altitude.
    float altitude;

    /// Temperature.
    float temperature;

    /// Sampling time of the data in seconds
    double timestamp;

    ZenHeaveMotionData hm;
} ZenImuData;

*/
    py::class_<ZenImuData>(m,"ZenImuData")
        .def_property_readonly("a", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.a);
        })
        .def_property_readonly("g", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.g);
        });

    // C++ part of the interface from OpenZen.h
    m.def("make_client", &make_client);    

    py::class_<ZenEvent>(m, "ZenEvent")
        .def_readonly("eventType", &ZenEvent::eventType)
        .def_readonly("sensor", &ZenEvent::sensor)
        .def_readonly("component", &ZenEvent::component)
        .def_readonly("data", &ZenEvent::data);

    py::class_<ZenClient>(m,"ZenClient")
        .def("obtainSensorByName", &ZenClient::obtainSensorByName)
        .def("pollNextEvent", &ZenClient::pollNextEvent)
        .def("waitForNextEvent", &ZenClient::waitForNextEvent);

    py::class_<ZenSensor>(m,"ZenSensor")
        .def("getAnyComponentOfType", &ZenSensor::getAnyComponentOfType)
        .def("getBoolProperty", &ZenSensor::getBoolProperty)
        .def("setBoolProperty", &ZenSensor::setBoolProperty);

    py::class_<ZenSensorComponent>(m,"ZenSensorComponent")
        .def("getBoolProperty", &ZenSensorComponent::getBoolProperty)
        .def("setBoolProperty", &ZenSensorComponent::setBoolProperty);
}

