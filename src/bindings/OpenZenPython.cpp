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

    py::class_<ZenImuData>(m,"ZenImuData")
        .def_property_readonly("a", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.a);
        }, "Calibrated accelerometer sensor data")
        .def_property_readonly("g", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.g);
        }, "Calibrated gyroscope sensor data")
        .def_property_readonly("b", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.b);
        }, "Calibrated magnetometer sensor data")
        .def_property_readonly("a_raw", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.aRaw);
        }, "Raw accelerometer sensor data")
        .def_property_readonly("g_raw", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.gRaw);
        }, "Raw gyroscope sensor data")
        .def_property_readonly("b_raw", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.bRaw);
        }, "Raw magnetometer sensor data")
        .def_property_readonly("w", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.w);
        }, "Angular velocity data")
        .def_property_readonly("r", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.r);
        }, "Euler angle data")
        .def_property_readonly("q", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 4>(data.q);
        }, "Quaternion orientation data. The component order is w, x, y, z")
        .def_property_readonly("rotation_m", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 9>(data.rotationM);
        }, "Orientation data as rotation matrix without offset")
        .def_property_readonly("rot_offset_m", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 9>(data.rotOffsetM);
        }, "Orientation data as rotation matrix after zeroing")
        .def_readonly("pressure", &ZenImuData::pressure,
            "Barometric pressure")
        .def_readonly("frame_count", &ZenImuData::frameCount,
            "Index of the data frame")
        .def_property_readonly("lin_acc", [](const ZenImuData & data) {
            return OpenZenPythonHelper::toStlArray<float, 3>(data.linAcc);
        }, "Linear acceleration x, y and z")
        .def_readonly("g_temp", &ZenImuData::gTemp,
            "Gyroscope temperature")
        .def_readonly("altitude", &ZenImuData::altitude,
            "Altitude")
        .def_readonly("temperature", &ZenImuData::temperature,
            "Temperature")
        .def_readonly("timestamp", &ZenImuData::timestamp,
            "Sampling time of the data in seconds")
        .def_readonly("heave_y", &ZenImuData::heaveY,
            "heave motion in y direction");

    py::enum_<ZenLogLevel>(m,"ZenLogLevel")
        .value("Off", ZenLogLevel_Off)
        .value("Error", ZenLogLevel_Error)
        .value("Warning", ZenLogLevel_Warning)
        .value("Info", ZenLogLevel_Info)
        .value("Debug", ZenLogLevel_Debug)
        .value("Max", ZenLogLevel_Max);

    py::class_<ZenEvent>(m, "ZenEvent")
        .def_readonly("event_type", &ZenEvent::eventType)
        .def_readonly("sensor", &ZenEvent::sensor)
        .def_readonly("component", &ZenEvent::component)
        .def_readonly("data", &ZenEvent::data);

    m.def("set_log_level", &ZenSetLogLevel, "Sets the loglevel to the console of the whole OpenZen library");

    // C++ part of the interface from OpenZen.h
    // starting here
    py::class_<ZenSensorComponent>(m,"ZenSensorComponent")
        .def_property_readonly("sensor", &ZenSensorComponent::sensor)
        .def_property_readonly("component", &ZenSensorComponent::component)
        .def_property_readonly("type", &ZenSensorComponent::type)

        .def("execute_property", &ZenSensorComponent::executeProperty)

        // get properties
        // array properties
        .def("get_array_property_float", &ZenSensorComponent::getArrayProperty<float>)
        .def("get_array_property_int32", &ZenSensorComponent::getArrayProperty<int32_t>)
        .def("get_array_property_byte", &ZenSensorComponent::getArrayProperty<std::byte>)
        .def("get_array_property_uint64", &ZenSensorComponent::getArrayProperty<uint64_t>)

        // scalar properties
        .def("get_bool_property", &ZenSensorComponent::getBoolProperty)
        .def("get_float_property", &ZenSensorComponent::getFloatProperty)
        .def("get_int32_property", &ZenSensorComponent::getInt32Property)
        .def("get_uint64_property", &ZenSensorComponent::getUInt64Property)

        // set properties
        // array properties
        .def("set_array_property_float", &ZenSensorComponent::setArrayProperty<float>)
        .def("set_array_property_int32", &ZenSensorComponent::setArrayProperty<int32_t>)
        .def("set_array_property_byte", &ZenSensorComponent::setArrayProperty<std::byte>)
        .def("set_array_property_uint64", &ZenSensorComponent::setArrayProperty<uint64_t>)

        // scalar properties
        .def("set_bool_property", &ZenSensorComponent::setBoolProperty)
        .def("set_float_property", &ZenSensorComponent::setFloatProperty)
        .def("set_int32_property", &ZenSensorComponent::setInt32Property)
        .def("set_uint64_property", &ZenSensorComponent::setUInt64Property)

        .def("forward_rtk_corrections", &ZenSensorComponent::forwardRtkCorrections);

    py::class_<ZenSensor>(m,"ZenSensor")
        .def("release", &ZenSensor::release)

        // updateFirmwareAsync and
        // updateIAPAsync not available via python interface at this time

        .def_property_readonly("io_type", &ZenSensor::ioType)
        .def("equals", &ZenSensor::equals)
        .def_property_readonly("sensor", &ZenSensor::sensor)
        .def("publish_events", &ZenSensor::publishEvents)
        .def("execute_property", &ZenSensor::executeProperty)

        .def("get_array_property_float", &ZenSensor::getArrayProperty<float>)
        .def("get_array_property_int32", &ZenSensor::getArrayProperty<int32_t>)
        .def("get_array_property_byte", &ZenSensor::getArrayProperty<std::byte>)
        .def("get_array_property_uint64", &ZenSensor::getArrayProperty<uint64_t>)
        .def("get_string_property", &ZenSensor::getStringProperty)

        .def_property_readonly("sensor", &ZenSensor::sensor)

        // scalar properties
        .def("get_bool_property", &ZenSensor::getBoolProperty)
        .def("get_float_property", &ZenSensor::getFloatProperty)
        .def("get_int32_property", &ZenSensor::getInt32Property)
        .def("get_uint64_property", &ZenSensor::getUInt64Property)

        // array property access
        .def("set_array_property_float", &ZenSensor::setArrayProperty<float>)
        .def("set_array_property_int32", &ZenSensor::setArrayProperty<int32_t>)
        .def("set_array_property_myte", &ZenSensor::setArrayProperty<std::byte>)
        .def("set_array_property_uint64", &ZenSensor::setArrayProperty<uint64_t>)

        .def("set_bool_property", &ZenSensor::setBoolProperty)
        .def("set_float_property", &ZenSensor::setFloatProperty)
        .def("set_int32_property", &ZenSensor::setInt32Property)
        .def("set_uint64_property", &ZenSensor::setUInt64Property)

        .def("get_any_component_of_type", &ZenSensor::getAnyComponentOfType);

    py::class_<ZenClient>(m,"ZenClient")
        .def("close", &ZenClient::close)
        .def("list_sensors_async", &ZenClient::listSensorsAsync)
        .def("obtain_sensor", &ZenClient::obtainSensor)
        .def("obtain_sensor_by_name", &ZenClient::obtainSensorByName)
        .def("poll_next_event", &ZenClient::pollNextEvent)
        .def("wait_for_next_event", &ZenClient::waitForNextEvent);

    m.def("make_client", &make_client);
}
