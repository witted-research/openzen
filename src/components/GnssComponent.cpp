//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "components/GnssComponent.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <spdlog/spdlog.h>

#include "ZenTypesHelpers.h"
#include "properties/ImuSensorPropertiesV0.h"
#include "components/SensorParsingUtil.h"

#include <iostream>

namespace zen
{
    GnssComponent::GnssComponent(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& com, unsigned int) noexcept
        : SensorComponent(std::move(properties)), m_communicator(com)
    {}

    ZenSensorInitError GnssComponent::init() noexcept
    {
        return ZenSensorInitError_None;
    }

    ZenError GnssComponent::processData(uint8_t /*function*/, gsl::span<const std::byte> /*data*/) noexcept
    {
        return ZenError_Io_UnsupportedFunction;
    }

    nonstd::expected<ZenEventData, ZenError> GnssComponent::processEventData(ZenEventType eventType, gsl::span<const std::byte> data) noexcept
    {
        switch (eventType)
        {
        case ZenEventType_GnssData:
            return parseSensorData(data);
            break;

        default:
            return nonstd::make_unexpected(ZenError_UnsupportedEvent);
        }
    }

    ZenError GnssComponent::storeGnssState() noexcept {
        if (ZenError_None != m_communicator.sendAndWaitForAck(0, uint8_t(EDevicePropertyV1::SaveGpsState),
            ZenProperty_t(EDevicePropertyV1::SaveGpsState),
            gsl::span<std::byte>()))
        {
            spdlog::error("Could not send command to Ig1 to presist data");

            return ZenError_Io_SendFailed;
        }
        spdlog::info("Command to presist Ig1 navigation data send.");

        return ZenError_None;
    }

    ZenError GnssComponent::close() noexcept {
        stopRtkCorrections();
        return storeGnssState();
    }

    ZenError GnssComponent::forwardRtkCorrections(RtkCorrectionSource correction,
        std::string const& hostname, unsigned long port) noexcept {
        if (correction == RtkCorrectionSource::RTCM3NetworkStream) {
            m_rtcm3network = std::make_unique<RTCM3NetworkSource>();

            m_rtcm3network->addFrameCallback([&](uint16_t messageType, std::vector<std::byte> const& frame) {
                spdlog::info("RTCM3 message type {0} size {1} of size received", messageType, frame.size());

                if (ZenError_None != m_communicator.sendAndWaitForAck(0, uint8_t(EDevicePropertyV1::SetRtkCorrection),
                    ZenProperty_t(EDevicePropertyV1::SetRtkCorrection), frame))
                {
                    spdlog::error("Could not send RTK correction to sensor");
                }
            });

            spdlog::info("Connecting to host {0}:{1} for RTK corrections", hostname, port);
            m_rtcm3network->start(hostname, port);
            return ZenError::ZenError_None;
        } else if(correction == RtkCorrectionSource::RTCM3SerialStream) {
                m_rtcm3serial = std::make_unique<RTCM3SerialSource>();

                m_rtcm3serial->addFrameCallback([&](uint16_t messageType, std::vector<std::byte> const& frame) {
                spdlog::info("RTCM3 message type {0} size {1} of size received", messageType, frame.size());

                if (ZenError_None != m_communicator.sendAndWaitForAck(0, uint8_t(EDevicePropertyV1::SetRtkCorrection),
                    ZenProperty_t(EDevicePropertyV1::SetRtkCorrection), frame))
                {
                    spdlog::error("Could not send RTK correction to sensor");
                }
            });

            spdlog::info("Connecting to serial {0}:{1} for RTK corrections", hostname, port);
            m_rtcm3serial->start(hostname, port);
            return ZenError::ZenError_None;
        }

        spdlog::error("Selected RTK correction source not supported");

        return ZenError::ZenError_InvalidArgument;
    }

    ZenError GnssComponent::stopRtkCorrections() noexcept {
        if (m_rtcm3network) {
            m_rtcm3network->stop();
            m_rtcm3network = nullptr;
        }
        if (m_rtcm3serial) {
            m_rtcm3serial->stop();
            m_rtcm3serial = nullptr;
        }
      return ZenError::ZenError_None;
   }


    nonstd::expected<ZenEventData, ZenError> GnssComponent::parseSensorData(gsl::span<const std::byte> data) const noexcept
    {
        ZenEventData eventData;
        ZenGnssData& gnssData = eventData.gnssData;
        gnssDataReset(gnssData);

        // check consistency of data package size
        if (data.size() < (ptrdiff_t)sizeof(uint32_t)) {
            spdlog::error("GPS data package size {0} too small, should at least contain the timestamp", data.size());
            return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
        }

        sensor_parsing_util::parseAndStoreScalar(data, &gnssData.frameCount);
        gnssData.timestamp = double(gnssData.frameCount) * 0.002;

        // most of the read out data are not transferred to the OpenZen data structure. Still we need to parse
        // the data buffer to end up at the right positions to read out the values we want.
        int32_t int32_not_used;
        uint32_t uint32_not_used;
        uint16_t uint16_not_used;
        uint8_t uint8_not_used;
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtiTOW),
            m_properties, data, &uint32_not_used);

        // date and time information
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtYear),
            m_properties, data, &gnssData.year);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtMonth),
            m_properties, data, &gnssData.month);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtDay),
            m_properties, data, &gnssData.day);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtHour),
            m_properties, data, &gnssData.hour);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtMinute),
            m_properties, data, &gnssData.minute);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtSecond),
            m_properties, data, &gnssData.second);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtValid),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvttAcc),
            m_properties, data, &uint32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtNano),
            m_properties, data, &gnssData.nanoSecondCorrection);

        uint8_t fixType = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtFixType),
            m_properties, data, &fixType)) {
            eventData.gnssData.fixType = ZenGnssFixType(fixType);
        }

        uint8_t navFlags = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtFlags),
            m_properties, data, &navFlags)) {
            // carrier phase solution in bit 7 and 8
            eventData.gnssData.carrierPhaseSolution = ZenGnssFixCarrierPhaseSolution( navFlags >> 6 );
        }

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtFlags2),
            m_properties, data, &uint8_not_used);

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtNumSV),
            m_properties, data, &gnssData.numberSatellitesUsed);

        int32_t longitude = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtLongitude),
            m_properties, data, &longitude)) {
            gnssData.longitude = sensor_parsing_util::integerToScaledDouble(longitude, -7);
        }

        int32_t latitude = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtLatitude),
            m_properties, data, &latitude))
        {
            gnssData.latitude = sensor_parsing_util::integerToScaledDouble(latitude, -7);
        }

        int32_t height = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtHeight),
            m_properties, data, &height)) {
            gnssData.height = sensor_parsing_util::integerToScaledDouble(height, -3);
        }

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvthMSL),
            m_properties, data, &int32_not_used );

        uint32_t horizontalAccuracy = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvthAcc),
            m_properties, data, &horizontalAccuracy)) {
            gnssData.horizontalAccuracy = sensor_parsing_util::integerToScaledDouble(horizontalAccuracy, -3);
        }
        
        uint32_t verticalAccuracy = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtvAcc),
            m_properties, data, &verticalAccuracy)) {
            gnssData.verticalAccuracy = sensor_parsing_util::integerToScaledDouble(verticalAccuracy, -3);
        }

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtVelN),
            m_properties, data, &int32_not_used );
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtVelE),
            m_properties, data, &int32_not_used );
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtVelD),
            m_properties, data, &int32_not_used );

        int32_t velocity = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtgSpeed),
            m_properties, data, &velocity)) {
            gnssData.velocity = sensor_parsing_util::integerToScaledDouble(velocity, -3);
        }

        int32_t headingOfMotion = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtHeadMot),
            m_properties, data, &headingOfMotion)) {
            gnssData.headingOfMotion = sensor_parsing_util::integerToScaledDouble(headingOfMotion, -5);
        }

        uint32_t velocityAccuracy = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtsAcc),
            m_properties, data, &velocityAccuracy)) {
            gnssData.velocityAccuracy = sensor_parsing_util::integerToScaledDouble(velocityAccuracy, -3);
        }

        int32_t headingAcc = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtHeadAcc),
            m_properties, data, &headingAcc)) {
            gnssData.headingAccuracy = sensor_parsing_util::integerToScaledDouble(headingAcc, -5);
        }

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtpDOP),
            m_properties, data, &uint16_not_used);
        
        int32_t headingOfVehicle = 0;
        if (sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavPvtHeadVeh),
            m_properties, data, &headingOfVehicle)) {
            gnssData.headingOfVehicle = sensor_parsing_util::integerToScaledDouble(headingOfVehicle, -5);
        }

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttiTOW),
            m_properties, data, &uint32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttVersion),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttRoll),
            m_properties, data, &int32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttPitch),
            m_properties, data, &int32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttHeading),
            m_properties, data, &int32_not_used);

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttAccRoll),
            m_properties, data, &uint32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttAccPitch),
            m_properties, data, &uint32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputNavAttAccHeading),
            m_properties, data, &uint32_not_used);

        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusiTOW),
            m_properties, data, &uint32_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusVersion),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusInitStatus1),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusInitStatus2),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusFusionMode),
            m_properties, data, &uint8_not_used);
        sensor_parsing_util::readScalarIfAvailable(static_cast<ZenProperty_t>(ZenGnssProperty_OutputEsfStatusNumSens),
            m_properties, data, &uint8_not_used);

        return eventData;
    }
}
