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
}

PYBIND11_MODULE(openzen, m) {

    // C part of the data types from ZenTypes.h
    py::enum_<ZenError>(m, "ZenError")
        .value("NoError", ZenError_None)
        .value("IsNull", ZenError_IsNull)
        .value("NotNull", ZenError_NotNull);

    py::enum_<EZenImuProperty>(m, "ZenImuProperty")
        .value("Invalid", ZenImuProperty_Invalid)
        .value("StreamData", ZenImuProperty_StreamData)

        .value("SamplingRate", ZenImuProperty_SamplingRate)
        .value("SupportedSamplingRates", ZenImuProperty_SupportedSamplingRates)

        .value("PollSensorData", ZenImuProperty_PollSensorData)
        .value("CalibrateGyro", ZenImuProperty_CalibrateGyro)
        .value("ResetOrientationOffset", ZenImuProperty_ResetOrientationOffset)

        .value("CentricCompensationRate", ZenImuProperty_CentricCompensationRate)
        .value("LinearCompensationRate", ZenImuProperty_LinearCompensationRate)

        .value("FieldRadius", ZenImuProperty_FieldRadius)
        .value("FilterMode", ZenImuProperty_FilterMode)
        .value("SupportedFilterModes", ZenImuProperty_SupportedFilterModes)
        .value("FilterPreset", ZenImuProperty_FilterPreset)

        .value("OrientationOffsetMode", ZenImuProperty_OrientationOffsetMode)

        .value("AccAlignment", ZenImuProperty_AccAlignment);

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


    py::class_<ZenSensorDesc>(m,"ZenSensorDesc")
        .def_readonly("name", &ZenSensorDesc::name)
        .def_readonly("io_type", &ZenSensorDesc::ioType);
/*        .def_property_readonly("complete", [](const ZenEventData_SensorListingProgress & data) -> bool {
            return data.complete > 0;
        });*/

    py::class_<ZenEventData_SensorDisconnected>(m,"SensorDisconnected")
        .def_readonly("error", &ZenEventData_SensorDisconnected::error);

    py::class_<ZenEventData_SensorListingProgress>(m,"SensorListingProgress")
        .def_readonly("progress", &ZenEventData_SensorListingProgress::progress)
        .def_property_readonly("complete", [](const ZenEventData_SensorListingProgress & data) -> bool {
            return data.complete > 0;
        });

    py::class_<ZenEventData>(m, "ZenEventData")
        .def_readonly("imu_data", &ZenEventData::imuData)
        .def_readonly("gnss_data", &ZenEventData::gnssData)
        .def_readonly("sensor_disconnected", &ZenEventData::sensorDisconnected)
        .def_readonly("sensor_found", &ZenEventData::sensorFound)
        .def_readonly("sensor_listing_progress", &ZenEventData::sensorListingProgress);

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


    py::enum_<ZenLogLevel>(m,"ZenLogLevel")
        .value("Off", ZenLogLevel_Off)
        .value("Error", ZenLogLevel_Error)
        .value("Warning", ZenLogLevel_Warning)
        .value("Info", ZenLogLevel_Info)
        .value("Debug", ZenLogLevel_Debug)
        .value("Max", ZenLogLevel_Max);

    m.def("set_log_level", &ZenSetLogLevel);

    // C++ part of the interface from OpenZen.h
    m.def("make_client", &make_client);

    py::class_<ZenEvent>(m, "ZenEvent")
        .def_readonly("event_type", &ZenEvent::eventType)
        .def_readonly("sensor", &ZenEvent::sensor)
        .def_readonly("component", &ZenEvent::component)
        .def_readonly("data", &ZenEvent::data);

    py::class_<ZenClient>(m,"ZenClient")
        .def("close", &ZenClient::close)
        .def("list_sensors_async", &ZenClient::listSensorsAsync)
        .def("obtain_sensor", &ZenClient::obtainSensor)
        .def("obtain_sensor_by_name", &ZenClient::obtainSensorByName)
        .def("poll_next_event", &ZenClient::pollNextEvent)
        .def("wait_for_next_event", &ZenClient::waitForNextEvent);

    py::class_<ZenSensor>(m,"ZenSensor")
        .def("release", &ZenSensor::release)
        .def_property_readonly("io_type", &ZenSensor::ioType)

        .def("publish_events", &ZenSensor::publishEvents)

        .def("get_any_component_of_type", &ZenSensor::getAnyComponentOfType)

        .def_property_readonly("sensor", &ZenSensor::sensor)

        .def("execute_property", &ZenSensor::executeProperty)

        // scalar properties
        .def("get_string_property", &ZenSensor::getStringProperty)
        .def("get_bool_property", &ZenSensor::getBoolProperty)
        .def("set_bool_property", &ZenSensor::setBoolProperty)
        .def("get_float_property", &ZenSensor::getFloatProperty)
        .def("set_float_property", &ZenSensor::setFloatProperty)
        .def("get_int32_property", &ZenSensor::getInt32Property)
        .def("set_int32_property", &ZenSensor::setInt32Property)
        .def("get_uint64_property", &ZenSensor::getUInt64Property)
        .def("set_uint64_property", &ZenSensor::setUInt64Property)

        // array property access
        .def("set_array_property_float", &ZenSensor::setArrayProperty<float>)
        .def("get_array_property_float", &ZenSensor::getArrayProperty<float>)
        .def("set_array_property_int32", &ZenSensor::setArrayProperty<int32_t>)
        .def("get_array_property_int32", &ZenSensor::getArrayProperty<int32_t>)
        .def("set_array_property_myte", &ZenSensor::setArrayProperty<std::byte>)
        .def("get_array_property_byte", &ZenSensor::getArrayProperty<std::byte>)
        .def("set_array_property_uint64", &ZenSensor::setArrayProperty<uint64_t>)
        .def("get_array_property_uint64", &ZenSensor::getArrayProperty<uint64_t>);

    py::class_<ZenSensorComponent>(m,"ZenSensorComponent")
        .def_property_readonly("type", &ZenSensorComponent::type)
        .def_property_readonly("sensor", &ZenSensorComponent::sensor)
        .def_property_readonly("component", &ZenSensorComponent::component)

        .def("execute_property", &ZenSensorComponent::executeProperty)

        // scalar properties
        .def("get_bool_property", &ZenSensorComponent::getBoolProperty)
        .def("set_bool_property", &ZenSensorComponent::setBoolProperty)
        .def("get_float_property", &ZenSensorComponent::getFloatProperty)
        .def("set_float_property", &ZenSensorComponent::setFloatProperty)
        .def("get_int32_property", &ZenSensorComponent::getInt32Property)
        .def("set_int32_property", &ZenSensorComponent::setInt32Property)
        .def("get_uint64_property", &ZenSensorComponent::getUInt64Property)
        .def("set_uint64_property", &ZenSensorComponent::setUInt64Property)

        // array property access
        .def("set_array_property_float", &ZenSensorComponent::setArrayProperty<float>)
        .def("get_array_property_float", &ZenSensorComponent::getArrayProperty<float>)
        .def("set_array_property_int32", &ZenSensorComponent::setArrayProperty<int32_t>)
        .def("get_array_property_int32", &ZenSensorComponent::getArrayProperty<int32_t>)
        .def("set_array_property_byte", &ZenSensorComponent::setArrayProperty<std::byte>)
        .def("get_array_property_byte", &ZenSensorComponent::getArrayProperty<std::byte>)
        .def("set_array_property_uint64", &ZenSensorComponent::setArrayProperty<uint64_t>)
        .def("get_array_property_uint64", &ZenSensorComponent::getArrayProperty<uint64_t>)

        .def("forward_rtk_corrections", &ZenSensorComponent::forwardRtkCorrections);
}
