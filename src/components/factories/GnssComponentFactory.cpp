//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "GnssComponentFactory.h"

#include <spdlog/spdlog.h>

#include "components/GnssComponent.h"

#include "InternalTypes.h"
#include "SensorProperties.h"
#include "properties/GnssPropertyRulesV1.h"
#include "properties/Ig1GnssProperties.h"
#include "properties/LegacyImuProperties.h"

namespace zen
{
    nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> GnssComponentFactory::make_component(
        unsigned int version,
        SpecialOptions,
        uint8_t,
        SyncedModbusCommunicator& communicator
    ) const noexcept
    {
        if (version == 1) {
            auto properties = std::make_unique<Ig1GnssProperties>(communicator);

            // Initialize to non-streaming to retrieve the config bitset
            if (ZenError_None != properties->setBool(ZenImuProperty_StreamData, false)) {
                spdlog::error("Cannot disable streaming of Ig1 sensor");
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }

            uint32_t gpsBitset[2];
            auto sendResult = communicator.sendAndWaitForArray(0u,
                static_cast<DeviceProperty_t>(EDevicePropertyV1::GetGpsTransmitData),
                static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigGpsOutputDataBitset), {},
                gsl::make_span(gpsBitset, sizeof(uint32_t) * 2));
            if (sendResult.first == ZenError_None)
            {
                const uint64_t gpsBitsetOut = (uint64_t(gpsBitset[1]) << 32) | uint64_t(gpsBitset[0]);
                properties->setGpsOutputDataBitset(gpsBitsetOut);
                return std::make_unique<GnssComponent>(std::move(properties), communicator, version);
            }
            else
            {
                spdlog::error("Cannot load GPS output bitset from sensor");
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }
        }

        spdlog::error("Unsupportded Sensor protocol for GNSS Component");
        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedProtocol);
    }
}
