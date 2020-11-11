//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "ImuComponentFactory.h"

#include <spdlog/spdlog.h>

#include "components/ImuComponent.h"
#include "components/ImuIg1Component.h"

#include "InternalTypes.h"
#include "SensorProperties.h"
#include "properties/ImuPropertyRulesV1.h"
#include "properties/ImuPropertyRulesV2.h"
#include "properties/Ig1ImuProperties.h"
#include "properties/LegacyImuProperties.h"

namespace zen
{
    std::unique_ptr<ISensorProperties> make_properties(unsigned int version, uint8_t id, SyncedModbusCommunicator& communicator)
    {
        switch (version)
        {
        case 1:
            return std::make_unique<SensorProperties<ImuPropertyRulesV1>>(id, communicator);

        default:
            return nullptr;
        }
    }

    nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> ImuComponentFactory::make_component(
        unsigned int version,
        SpecialOptions specialOptions,
        uint8_t id,
        SyncedModbusCommunicator& communicator
    ) const noexcept
    {
        // Legacy sensors require a "Command Mode" to active for accessing properties
        // as well a configuration bitset to determine which data to output
        if (version == 0)
        {
            auto properties = std::make_unique<LegacyImuProperties>(communicator);

            // Initialize to non-streaming to retrieve the config bitset
            if (ZenError_None != properties->setBool(ZenImuProperty_StreamData, false)) {
                spdlog::debug("Cannot disable streaming of legacy sensor");
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }

            if (auto bitset = communicator.sendAndWaitForResult<uint32_t>(0u, static_cast<DeviceProperty_t>(EDevicePropertyInternal::ConfigImuOutputDataBitset),
                static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigImuOutputDataBitset), {}))
            {
                spdlog::debug("Loaded config bitset of legacy sensor: {}", bitset.value());
                properties->setConfigBitset(*bitset);
                return std::make_unique<ImuComponent>(std::move(properties), communicator, version);
            }
            else
            {
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }
        }
        else if (version == 1) {
            auto properties = std::make_unique<Ig1ImuProperties>(communicator);

            // Initialize to non-streaming to retrieve the config bitset
            if (ZenError_None != properties->setBool(ZenImuProperty_StreamData, false)) {
                spdlog::debug("Cannot disable streaming of Ig1 sensor");
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }

            if (auto bitset = communicator.sendAndWaitForResult<uint32_t>(0u, static_cast<DeviceProperty_t>(EDevicePropertyV1::GetImuTransmitData),
                static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigImuOutputDataBitset), {}))
            {
                spdlog::debug("Loaded output bitset of Ig1 sensor: {}", bitset.value());
                properties->setOutputDataBitset(*bitset);
            }
            else
            {
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }

            if (auto degreeOutputConfigured =
                communicator.sendAndWaitForResult<uint32_t>(0u, static_cast<DeviceProperty_t>(EDevicePropertyV1::GetDegGradOutput),
                static_cast<ZenProperty_t>(EDevicePropertyInternal::ConfigGetDegGradOutput), {}))
            {
                spdlog::debug("Ig1 sensor outputs degrees: {}", degreeOutputConfigured.value() == 0 );
                properties->setDegGradOutput(degreeOutputConfigured.value() > 0);
            }
            else
            {
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }

            bool useSecondGyroAsPrimary = specialOptions & SpecialOptions_SecondGyroIsPrimary;

            return std::make_unique<ImuIg1Component>(std::move(properties), communicator, version, useSecondGyroAsPrimary);

        }

        if (auto properties = make_properties(version, id, communicator))
            return std::make_unique<ImuComponent>(std::move(properties), communicator, version);

        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedProtocol);
    }
}