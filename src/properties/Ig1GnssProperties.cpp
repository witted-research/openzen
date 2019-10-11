#include "properties/Ig1GnssProperties.h"

#include "properties/ImuSensorPropertiesV1.h"

#include "SensorProperties.h"
#include "InternalTypes.h"
#include "utility/Finally.h"

#include <spdlog/spdlog.h>

#include <math.h>
#include <iostream>
#include <map>
#include <optional>

namespace zen
{
    namespace
    {
        // change here for the new output flags of IG1
/*
not supported by Ig1 atm
template <> struct OutputDataFlag<ZenImuProperty_OutputLowPrecision>
        {
            using index = std::integral_constant<unsigned int, 22>;
        };
        */

        static std::map< ZenProperty_t, unsigned int> outputGpsDataFlagMapping = {
            {ZenGnnsProperty_OutputNavPvtiTOW,0},
            {ZenGnnsProperty_OutputNavPvtYear,1},
            {ZenGnnsProperty_OutputNavPvtMonth,2},

            {ZenGnnsProperty_OutputNavPvtDay,3},
            {ZenGnnsProperty_OutputNavPvtHour,4},
            {ZenGnnsProperty_OutputNavPvtMinute,5},
            {ZenGnnsProperty_OutputNavPvtSecond,6},
            {ZenGnnsProperty_OutputNavPvtValid,7},
            {ZenGnnsProperty_OutputNavPvttAcc,8},

            {ZenGnnsProperty_OutputNavPvtNano,9},
            {ZenGnnsProperty_OutputNavPvtFixType,10},
            {ZenGnnsProperty_OutputNavPvtFlags,11},
            {ZenGnnsProperty_OutputNavPvtFlags2,12},
            {ZenGnnsProperty_OutputNavPvtNumSV,13},

            {ZenGnnsProperty_OutputNavPvtLongitude,14},
            {ZenGnnsProperty_OutputNavPvtLatitude,15},
            {ZenGnnsProperty_OutputNavPvtHeight,16},
            {ZenGnnsProperty_OutputNavPvthMSL,17},
            {ZenGnnsProperty_OutputNavPvthAcc,18},
            {ZenGnnsProperty_OutputNavPvtvAcc,19},
            {ZenGnnsProperty_OutputNavPvtVelN,20},
            {ZenGnnsProperty_OutputNavPvtVelE,21},
            {ZenGnnsProperty_OutputNavPvtVelD,22},
            {ZenGnnsProperty_OutputNavPvtgSpeed,23},
            {ZenGnnsProperty_OutputNavPvtHeadMot,24},
            {ZenGnnsProperty_OutputNavPvtsAcc,25},
            {ZenGnnsProperty_OutputNavPvtHeadAcc,26},
            {ZenGnnsProperty_OutputNavPvtpDOP,27},
            {ZenGnnsProperty_OutputNavPvtHeadVeh,28},

            // these ones are in the second 32-bit part of the 64-bit
            {ZenGnnsProperty_OutputNavAttiTOW, 32 + 0},
            {ZenGnnsProperty_OutputNavAttVersion,32 + 1},
            {ZenGnnsProperty_OutputNavAttRoll,32 + 2},
            {ZenGnnsProperty_OutputNavAttPitch,32 + 3},
            {ZenGnnsProperty_OutputNavAttHeading,32 + 4},
            {ZenGnnsProperty_OutputNavAttAccRoll,32 + 5},
            {ZenGnnsProperty_OutputNavAttAccPitch,32 + 6},
            {ZenGnnsProperty_OutputNavAttAccHeading,32 + 7},
            {ZenGnnsProperty_OutputEsfStatusiTOW,32 + 8},
            {ZenGnnsProperty_OutputEsfStatusVersion,32 + 9},
            {ZenGnnsProperty_OutputEsfStatusInitStatus1,32 + 10},
            {ZenGnnsProperty_OutputEsfStatusInitStatus2,32 + 11},
            {ZenGnnsProperty_OutputEsfStatusFusionMode,32 + 12},
            {ZenGnnsProperty_OutputEsfStatusNumSens,32 + 13},
            {ZenGnnsProperty_OutputEsfStatusSensStatus,32 + 14}
        };

        std::optional<bool> getOutputDataFlag(ZenProperty_t property, std::atomic_uint64_t& outputDataBitset) noexcept
        {
            try {
                auto flag_position = outputGpsDataFlagMapping.at(property);

                return (outputDataBitset & (uint64_t(1) << flag_position)) != 0;
            } catch (std::out_of_range &) {
                spdlog::error("ZenProperty {0} not known for OutputGpsFlag", property);
                return std::nullopt;
            }
        }

        ZenError setAndSendGpsOutputDataBitset(Ig1GnssProperties& self, SyncedModbusCommunicator& communicator,
            ZenProperty_t property,
            std::atomic_uint64_t& outputDataBitset, std::function<void(ZenProperty_t,SensorPropertyValue)> notifyPropertyChange,
            bool streaming, bool value) noexcept
        {
            if (streaming)
                if (auto error = self.setBool(ZenImuProperty_StreamData, false))
                    return error;

            auto guard = finally([&self, streaming]() {
                if (streaming)
                    self.setBool(ZenImuProperty_StreamData, true);
                });

                try {
                    unsigned int flag_location = outputGpsDataFlagMapping.at(property);
                    uint64_t newBitset;
                    if (value)
                        newBitset = outputDataBitset | (uint64_t(1) << flag_location);
                    else
                        newBitset = outputDataBitset & ~(uint64_t(1) << flag_location);

                    // split the 64-bit set onto 2 32-bit sets
                    uint32_t newBitsetArray[2] = {
                        uint32_t(newBitset),
                        uint32_t(newBitset >> 32)
                    };

                    if (auto error = communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(EDevicePropertyV1::SetGpsTransmitData),
                        static_cast<ZenProperty_t>(EDevicePropertyV1::SetGpsTransmitData),
                        gsl::make_span(reinterpret_cast<const std::byte*>(&newBitsetArray), sizeof(newBitsetArray))))
                        return error;

                    outputDataBitset = newBitset;
                    notifyPropertyChange(property, value);
                } catch (std::out_of_range &) {
                        return ZenError_UnknownProperty;
                }
            return ZenError_None;
        }
    }

    Ig1GnssProperties::Ig1GnssProperties(SyncedModbusCommunicator& communicator) noexcept
        : m_cache{}
        , m_communicator(communicator)
        , m_streaming(true)
    {
    }

    ZenError Ig1GnssProperties::execute(ZenProperty_t command) noexcept
    {
        if (isExecutable(command))
        {
            if (auto streaming = getBool(ZenImuProperty_StreamData))
            {
                if (*streaming)
                    if (auto error = setBool(ZenImuProperty_StreamData, false))
                        return error;

                auto guard = finally([&]() {
                    if (*streaming)
                        setBool(ZenImuProperty_StreamData, true);
                });

                const auto function = static_cast<DeviceProperty_t>(imu::v1::mapCommand(command));
                return m_communicator.sendAndWaitForAck(0, function, function, {});
            }
            else
            {
                return streaming.error();
            }
        }

        return ZenError_UnknownProperty;
    }

    nonstd::expected<bool, ZenError> Ig1GnssProperties::getBool(ZenProperty_t property) noexcept
    {
        if (property == ZenImuProperty_StreamData)
            return m_streaming;

        auto outputFlag = getOutputDataFlag(property, m_cache.outputGpsDataBitset);

        if (!outputFlag.has_value()) {
            return nonstd::make_unexpected(ZenError_UnknownProperty);
        } else {
            return *outputFlag;
        }
    }

    ZenError Ig1GnssProperties::setBool(ZenProperty_t property, bool value) noexcept
    {
        if (property == ZenImuProperty_StreamData)
        {
            if (m_streaming != value)
            {
                const auto propertyV0 = value ? EDevicePropertyV1::GotoStreamMode : EDevicePropertyV1::GotoCommandMode;
                if (auto error = m_communicator.sendAndWaitForAck(0, static_cast<DeviceProperty_t>(propertyV0),
                    static_cast<ZenProperty_t>(propertyV0),
                    gsl::span<std::byte>()
                    )) {
                    return error;
                }
                m_streaming = value;
                notifyPropertyChange(property, value);
            }
            return ZenError_None;
        } else {
            return setAndSendGpsOutputDataBitset(*this, m_communicator, property,  m_cache.outputGpsDataBitset,
                [=](ZenProperty_t property, SensorPropertyValue value) { notifyPropertyChange(property, value); },
                    m_streaming, value);
        }
    }

    ZenPropertyType Ig1GnssProperties::type(ZenProperty_t property) const noexcept
    {
        if (property == ZenImuProperty_StreamData) {
            return ZenPropertyType_Bool;
        }

        // all our other properties are bool
        if (outputGpsDataFlagMapping.find(property) != outputGpsDataFlagMapping.end()) {
            return ZenPropertyType_Bool;
        }

        return ZenPropertyType_Invalid;
    }
}
