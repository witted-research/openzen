//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

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

    py::class_<ZenClientHandle>(m, "ZenClientHandle")
        .def_readonly("handle", &ZenClientHandle::handle)
        .def("__eq__", [](ZenClientHandle const & self, ZenClientHandle const & other) {
            return self.handle == other.handle;}, py::is_operator())
        .def("__ne__", [](ZenClientHandle const & self, ZenClientHandle const & other) {
            return self.handle != other.handle;}, py::is_operator());

    py::class_<ZenSensorHandle>(m, "ZenSensorHandle")
        .def_readonly("handle", &ZenSensorHandle::handle)
        .def("__eq__", [](ZenSensorHandle const & self, ZenSensorHandle const & other) {
            return self.handle == other.handle;}, py::is_operator())
        .def("__ne__", [](ZenSensorHandle const & self, ZenSensorHandle const & other) {
            return self.handle != other.handle;}, py::is_operator());

    py::class_<ZenComponentHandle>(m, "ZenComponentHandle")
        .def_readonly("handle", &ZenComponentHandle::handle)
        .def("__eq__", [](ZenComponentHandle const & self, ZenComponentHandle const & other) {
            return self.handle == other.handle;}, py::is_operator())
        .def("__ne__", [](ZenComponentHandle const & self, ZenComponentHandle const & other) {
            return self.handle != other.handle;}, py::is_operator());

    // C part of the data types from ZenTypes.h
    py::enum_<ZenError>(m, "ZenError")
        .value("NoError", ZenError_None)
        .value("Unknown", ZenError_Unknown)

        .value("IsNull", ZenError_IsNull)
        .value("NotNull", ZenError_NotNull)
        .value("WrongDataType", ZenError_WrongDataType)
        .value("BufferTooSmall", ZenError_BufferTooSmall)
        .value("InvalidArgument", ZenError_InvalidArgument)
        .value("NotSupported", ZenError_NotSupported)

        .value("AlreadyInitialized", ZenError_AlreadyInitialized)
        .value("NotInitialized", ZenError_NotInitialized)

        .value("Device_IoTypeInvalid", ZenError_Device_IoTypeInvalid)
        .value("Sensor_VersionNotSupported", ZenError_Sensor_VersionNotSupported)
        .value("Device_ListingFailed", ZenError_Device_ListingFailed)
        .value("Device_Listing", ZenError_Device_Listing)

        .value("WrongSensorType", ZenError_WrongSensorType)
        .value("WrongIoType", ZenError_WrongIoType)
        .value("UnknownDeviceId", ZenError_UnknownDeviceId)

        .value("Io_AlreadyInitialized", ZenError_Io_AlreadyInitialized)
        .value("Io_NotInitialized", ZenError_Io_NotInitialized)
        .value("Io_InitFailed", ZenError_Io_InitFailed)
        .value("Io_DeinitFailed", ZenError_Io_DeinitFailed)
        .value("Io_ReadFailed", ZenError_Io_ReadFailed)
        .value("Io_SendFailed", ZenError_Io_SendFailed)
        .value("Io_GetFailed", ZenError_Io_GetFailed)
        .value("Io_SetFailed", ZenError_Io_SetFailed)
        .value("Io_Busy", ZenError_Io_Busy)
        .value("Io_Timeout", ZenError_Io_Timeout)
        .value("Io_UnexpectedFunction", ZenError_Io_UnexpectedFunction)
        .value("Io_UnsupportedFunction", ZenError_Io_UnsupportedFunction)
        .value("Io_MsgCorrupt", ZenError_Io_MsgCorrupt)
        .value("Io_MsgTooBig", ZenError_Io_MsgTooBig)
        .value("Io_ExpectedAck", ZenError_Io_ExpectedAck)
        .value("Io_BaudratesUnknown", ZenError_Io_BaudratesUnknown)

        .value("UnknownProperty", ZenError_UnknownProperty)
        .value("UnknownCommandMode", ZenError_UnknownCommandMode)
        .value("UnsupportedEvent", ZenError_UnsupportedEvent)

        .value("FW_FunctionFailed", ZenError_FW_FunctionFailed)

        .value("Can_BusError", ZenError_Can_BusError)
        .value("Can_OutOfAddresses", ZenError_Can_OutOfAddresses)
        .value("Can_ResetFailed", ZenError_Can_ResetFailed)
        .value("Can_AddressOutOfRange", ZenError_Can_AddressOutOfRange)

        .value("InvalidClientHandle", ZenError_InvalidClientHandle)
        .value("InvalidSensorHandle", ZenError_InvalidSensorHandle)
        .value("InvalidComponentHandle", ZenError_InvalidComponentHandle);

    py::enum_<ZenSensorInitError>(m, "ZenSensorInitError")
        .value("NoError", ZenSensorInitError_None)

        .value("InvalidHandle", ZenSensorInitError_InvalidHandle)
        .value("IsNull", ZenSensorInitError_IsNull)
        .value("UnknownIdentifier", ZenSensorInitError_UnknownIdentifier)
        .value("UnsupportedComponent", ZenSensorInitError_UnsupportedComponent)
        .value("UnsupportedDataFormat", ZenSensorInitError_UnsupportedDataFormat)
        .value("UnsupportedIoType", ZenSensorInitError_UnsupportedIoType)
        .value("UnsupportedProtocol", ZenSensorInitError_UnsupportedProtocol)
        .value("UnsupportedFunction", ZenSensorInitError_UnsupportedFunction)

        .value("ConnectFailed", ZenSensorInitError_ConnectFailed)
        .value("IoFailed", ZenSensorInitError_IoFailed)
        .value("RetrieveFailed", ZenSensorInitError_RetrieveFailed)
        .value("SetBaudRateFailed", ZenSensorInitError_SetBaudRateFailed)
        .value("SendFailed", ZenSensorInitError_SendFailed)
        .value("Timeout", ZenSensorInitError_Timeout)
        .value("IncompatibleBaudRates", ZenSensorInitError_IncompatibleBaudRates)
        .value("InvalidAddress", ZenSensorInitError_InvalidAddress)
        .value("InvalidConfig", ZenSensorInitError_InvalidConfig)
        .value("NoConfiguration", ZenSensorInitError_NoConfiguration);

    // ZenAsyncStatus not added to binding because only used by
    // firmware update methods which are not supported from the
    // python API

    py::enum_<ZenLogLevel>(m,"ZenLogLevel")
        .value("Off", ZenLogLevel_Off)
        .value("Error", ZenLogLevel_Error)
        .value("Warning", ZenLogLevel_Warning)
        .value("Info", ZenLogLevel_Info)
        .value("Debug", ZenLogLevel_Debug)
        .value("Max", ZenLogLevel_Max);

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
        .def_readonly("heave_motion", &ZenImuData::heaveMotion,
            "heave motion (not supported by all sensor firmware versions)");

    py::enum_<ZenGnssFixType>(m, "ZenGnssFixType")
        .value("NoFix", ZenGnssFixType_NoFix)
        .value("DeadReckoningOnly", ZenGnssFixType_DeadReckoningOnly)
        .value("TwoDimensionalFix", ZenGnssFixType_2dFix)
        .value("ThreeDimonsionalFix", ZenGnssFixType_3dFix)
        .value("GnssAndDeadReckoning", ZenGnssFixType_GnssAndDeadReckoning)
        .value("TimeOnlyFix", ZenGnssFixType_TimeOnlyFix);

    py::enum_<ZenGnssFixCarrierPhaseSolution>(m, "ZenGnssFixCarrierPhaseSolution")
        .value("NoSolution", ZenGnssFixCarrierPhaseSolution_None)
        .value("FloatAmbiguities", ZenGnssFixCarrierPhaseSolution_FloatAmbiguities)
        .value("FixedAmbiguities", ZenGnssFixCarrierPhaseSolution_FixedAmbiguities);

    py::class_<ZenGnssData>(m,"ZenGnssData")
        .def_readonly("frameCount", &ZenGnssData::frameCount)
        .def_readonly("timestamp", &ZenGnssData::timestamp)
        .def_readonly("latitude", &ZenGnssData::latitude)
        .def_readonly("horizontal_accuracy", &ZenGnssData::horizontalAccuracy)
        .def_readonly("longitude", &ZenGnssData::longitude)
        .def_readonly("vertical_accuracy", &ZenGnssData::verticalAccuracy)
        .def_readonly("height", &ZenGnssData::height)
        .def_readonly("heading_of_motion", &ZenGnssData::headingOfMotion)
        .def_readonly("heading_of_vehicle", &ZenGnssData::headingOfVehicle)
        .def_readonly("heading_accuracy", &ZenGnssData::headingAccuracy)
        .def_readonly("velocity", &ZenGnssData::velocity)
        .def_readonly("velocity_accuracy", &ZenGnssData::velocityAccuracy)
        .def_readonly("fix_type", &ZenGnssData::fixType)
        .def_readonly("carrier_phase_solution", &ZenGnssData::carrierPhaseSolution)
        .def_readonly("number_satellites_used", &ZenGnssData::numberSatellitesUsed)
        .def_readonly("year", &ZenGnssData::year)
        .def_readonly("month", &ZenGnssData::month)
        .def_readonly("day", &ZenGnssData::day)
        .def_readonly("hour", &ZenGnssData::hour)
        .def_readonly("minute", &ZenGnssData::minute)
        .def_readonly("second", &ZenGnssData::second)
        .def_readonly("nano_second_correction", &ZenGnssData::nanoSecondCorrection);

    py::class_<ZenSensorDesc>(m,"ZenSensorDesc")
        .def_readonly("name", &ZenSensorDesc::name)
        .def_readonly("serial_number", &ZenSensorDesc::serialNumber)
        .def_readonly("io_type", &ZenSensorDesc::ioType)
        .def_readonly("identifier", &ZenSensorDesc::identifier)
        .def_readonly("baud_rate", &ZenSensorDesc::baudRate);

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

    py::enum_<ZenEventType>(m, "ZenEventType")
        .value("NoType", ZenEventType_None)
        .value("SensorFound", ZenEventType_SensorFound)
        .value("SensorListingProgress", ZenEventType_SensorListingProgress)
        .value("SensorDisconnected", ZenEventType_SensorDisconnected)
        .value("ImuData", ZenEventType_ImuData)
        .value("GnssData", ZenEventType_GnssData);

    py::class_<ZenEvent>(m, "ZenEvent")
        .def_readonly("event_type", &ZenEvent::eventType)
        .def_readonly("sensor", &ZenEvent::sensor)
        .def_readonly("component", &ZenEvent::component)
        .def_readonly("data", &ZenEvent::data);

    py::enum_<EZenSensorProperty>(m, "ZenSensorProperty")
        .value("DeviceName", ZenSensorProperty_DeviceName)
        .value("FirmwareInfo", ZenSensorProperty_FirmwareInfo)
        .value("FirmwareVersion", ZenSensorProperty_FirmwareVersion)
        .value("SerialNumber", ZenSensorProperty_SerialNumber)
        .value("RestoreFactorySettings", ZenSensorProperty_RestoreFactorySettings)
        .value("StoreSettingsInFlash", ZenSensorProperty_StoreSettingsInFlash)

        .value("BatteryCharging", ZenSensorProperty_BatteryCharging)
        .value("BatteryLevel", ZenSensorProperty_BatteryLevel)
        .value("BatteryVoltage", ZenSensorProperty_BatteryVoltage)

        .value("BaudRate", ZenSensorProperty_BaudRate)
        .value("SupportedBaudRates", ZenSensorProperty_SupportedBaudRates)

        .value("DataMode", ZenSensorProperty_DataMode)
        .value("TimeOffset", ZenSensorProperty_TimeOffset)

        .value("SensorModel", ZenSensorProperty_SensorModel);

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

        .value("AccAlignment", ZenImuProperty_AccAlignment)
        .value("AccBias", ZenImuProperty_AccBias)
        .value("AccRange", ZenImuProperty_AccRange)
        .value("AccSupportedRanges", ZenImuProperty_AccSupportedRanges)

        .value("GyrAlignment", ZenImuProperty_GyrAlignment)
        .value("GyrBias", ZenImuProperty_GyrBias)
        .value("GyrRange", ZenImuProperty_GyrRange)
        .value("GyrSupportedRanges", ZenImuProperty_GyrSupportedRanges)
        .value("GyrUseAutoCalibration", ZenImuProperty_GyrUseAutoCalibration)
        .value("GyrUseThreshold", ZenImuProperty_GyrUseThreshold)

        .value("MagAlignment", ZenImuProperty_MagAlignment)
        .value("MagBias", ZenImuProperty_MagBias)
        .value("MagRange", ZenImuProperty_MagRange)
        .value("MagSupportedRanges", ZenImuProperty_MagSupportedRanges)
        .value("MagReference", ZenImuProperty_MagReference)
        .value("MagHardIronOffset", ZenImuProperty_MagHardIronOffset)
        .value("MagSoftIronMatrix", ZenImuProperty_MagSoftIronMatrix)

        .value("OutputLowPrecision", ZenImuProperty_OutputLowPrecision)
        .value("OutputRawAcc", ZenImuProperty_OutputRawAcc)
        .value("OutputRawGyr", ZenImuProperty_OutputRawGyr)
        .value("OutputRawMag", ZenImuProperty_OutputRawMag)
        .value("OutputEuler", ZenImuProperty_OutputEuler)
        .value("OutputQuat", ZenImuProperty_OutputQuat)
        .value("OutputAngularVel", ZenImuProperty_OutputAngularVel)
        .value("OutputLinearAcc", ZenImuProperty_OutputLinearAcc)
        .value("OutputHeaveMotion", ZenImuProperty_OutputHeaveMotion)
        .value("OutputAltitude", ZenImuProperty_OutputAltitude)
        .value("OutputPressure", ZenImuProperty_OutputPressure)
        .value("OutputTemperature", ZenImuProperty_OutputTemperature)

        .value("OutputAccCalibrated", ZenImuProperty_OutputAccCalibrated)
        .value("OutputRawGyr0", ZenImuProperty_OutputRawGyr0)
        .value("OutputRawGyr1", ZenImuProperty_OutputRawGyr1)
        .value("OutputGyr0BiasCalib", ZenImuProperty_OutputGyr0BiasCalib)
        .value("OutputGyr1BiasCalib", ZenImuProperty_OutputGyr1BiasCalib)
        .value("OutputGyr0AlignCalib", ZenImuProperty_OutputGyr0AlignCalib)
        .value("OutputGyr1AlignCalib", ZenImuProperty_OutputGyr1AlignCalib)
        .value("OutputMagCalib", ZenImuProperty_OutputMagCalib)

        .value("DegRadOutput", ZenImuProperty_DegRadOutput)

        .value("CanChannelMode", ZenImuProperty_CanChannelMode)
        .value("CanPointMode", ZenImuProperty_CanPointMode)
        .value("CanStartId", ZenImuProperty_CanStartId)
        .value("CanBaudrate", ZenImuProperty_CanBaudrate)
        .value("CanMapping", ZenImuProperty_CanMapping)
        .value("CanHeartbeat", ZenImuProperty_CanHeartbeat)

        .value("UartBaudRate", ZenImuProperty_UartBaudRate)
        .value("UartFormat", ZenImuProperty_UartFormat)

        .value("StartSensorSync", ZenImuProperty_StartSensorSync)
        .value("StopSensorSync", ZenImuProperty_StopSensorSync);

    py::enum_<EZenGnssProperty>(m, "ZenGnssProperty")
        .value("Invalid", ZenGnssProperty_Invalid)

        .value("OutputNavPvtiTOW", ZenGnssProperty_OutputNavPvtiTOW)
        .value("OutputNavPvtYear", ZenGnssProperty_OutputNavPvtYear)
        .value("OutputNavPvtMonth", ZenGnssProperty_OutputNavPvtMonth)
        .value("OutputNavPvtDay", ZenGnssProperty_OutputNavPvtDay)
        .value("OutputNavPvtHour", ZenGnssProperty_OutputNavPvtHour)
        .value("OutputNavPvtMinute", ZenGnssProperty_OutputNavPvtMinute)
        .value("OutputNavPvtSecond", ZenGnssProperty_OutputNavPvtSecond)
        .value("OutputNavPvtValid", ZenGnssProperty_OutputNavPvtValid)
        .value("OutputNavPvttAcc", ZenGnssProperty_OutputNavPvttAcc)
        .value("OutputNavPvtNano", ZenGnssProperty_OutputNavPvtNano)
        .value("OutputNavPvtFixType", ZenGnssProperty_OutputNavPvtFixType)
        .value("OutputNavPvtFlags", ZenGnssProperty_OutputNavPvtFlags)
        .value("OutputNavPvtFlags2", ZenGnssProperty_OutputNavPvtFlags2)
        .value("OutputNavPvtNumSV", ZenGnssProperty_OutputNavPvtNumSV)
        .value("OutputNavPvtLongitude", ZenGnssProperty_OutputNavPvtLongitude)
        .value("OutputNavPvtLatitude", ZenGnssProperty_OutputNavPvtLatitude)
        .value("OutputNavPvtHeight", ZenGnssProperty_OutputNavPvtHeight)
        .value("OutputNavPvthMSL", ZenGnssProperty_OutputNavPvthMSL)
        .value("OutputNavPvthAcc", ZenGnssProperty_OutputNavPvthAcc)
        .value("OutputNavPvtvAcc", ZenGnssProperty_OutputNavPvtvAcc)
        .value("OutputNavPvtVelN", ZenGnssProperty_OutputNavPvtVelN)
        .value("OutputNavPvtVelE", ZenGnssProperty_OutputNavPvtVelE)
        .value("OutputNavPvtVelD", ZenGnssProperty_OutputNavPvtVelD)
        .value("OutputNavPvtgSpeed", ZenGnssProperty_OutputNavPvtgSpeed)
        .value("OutputNavPvtHeadMot", ZenGnssProperty_OutputNavPvtHeadMot)
        .value("OutputNavPvtsAcc", ZenGnssProperty_OutputNavPvtsAcc)
        .value("OutputNavPvtHeadAcc", ZenGnssProperty_OutputNavPvtHeadAcc)
        .value("OutputNavPvtpDOP", ZenGnssProperty_OutputNavPvtpDOP)
        .value("OutputNavPvtHeadVeh", ZenGnssProperty_OutputNavPvtHeadVeh)

        .value("OutputNavAttiTOW", ZenGnssProperty_OutputNavAttiTOW)
        .value("OutputNavAttVersion", ZenGnssProperty_OutputNavAttVersion)
        .value("OutputNavAttRoll", ZenGnssProperty_OutputNavAttRoll)
        .value("OutputNavAttPitch", ZenGnssProperty_OutputNavAttPitch)
        .value("OutputNavAttHeading", ZenGnssProperty_OutputNavAttHeading)
        .value("OutputNavAttAccRoll", ZenGnssProperty_OutputNavAttAccRoll)
        .value("OutputNavAttAccPitch", ZenGnssProperty_OutputNavAttAccPitch)
        .value("OutputNavAttAccHeading", ZenGnssProperty_OutputNavAttAccHeading)

        .value("OutputEsfStatusiTOW", ZenGnssProperty_OutputEsfStatusiTOW)
        .value("OutputEsfStatusVersion", ZenGnssProperty_OutputEsfStatusVersion)
        .value("OutputEsfStatusInitStatus1", ZenGnssProperty_OutputEsfStatusInitStatus1)
        .value("OutputEsfStatusInitStatus2", ZenGnssProperty_OutputEsfStatusInitStatus2)
        .value("OutputEsfStatusFusionMode", ZenGnssProperty_OutputEsfStatusFusionMode)
        .value("OutputEsfStatusNumSens", ZenGnssProperty_OutputEsfStatusNumSens)
        .value("OutputEsfStatusSensStatus", ZenGnssProperty_OutputEsfStatusSensStatus);

    py::enum_<ZenOrientationOffsetMode>(m, "ZenOrientationOffsetMode")
        .value("Object", ZenOrientationOffsetMode_Object)
        .value("Heading", ZenOrientationOffsetMode_Heading)
        .value("Alignment", ZenOrientationOffsetMode_Alignment);

    // ZenPropertyType enum is not needed in the Python bindings because
    // we have dedicated methods for each array type in python

    m.attr("component_type_imu") = g_zenSensorType_Imu;
    m.attr("component_type_gnss") = g_zenSensorType_Gnss;

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
        .def("obtain_sensor_by_name", &ZenClient::obtainSensorByName,
             py::arg("ioType"), py::arg("identifier"), py::arg("baudrate") = 0)
        .def("poll_next_event", &ZenClient::pollNextEvent)
        .def("wait_for_next_event", &ZenClient::waitForNextEvent);

    m.def("make_client", &make_client);
}
