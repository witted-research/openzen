//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSORCOMPONENT_H_
#define ZEN_SENSORCOMPONENT_H_

#include <memory>
#include <string_view>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "ISensorProperties.h"

namespace zen
{
    class SensorComponent
    {
    public:
        SensorComponent(std::unique_ptr<ISensorProperties> properties) noexcept
            : m_properties(std::move(properties))
        {}

        virtual ~SensorComponent() noexcept = default;

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        virtual ZenSensorInitError init() noexcept = 0;

        /**
        Called before communication to the component is terminated
        */
        virtual ZenError close() noexcept { return ZenError_None; }

        virtual ZenError processData(uint8_t function, gsl::span<const std::byte> data) noexcept = 0;
        virtual nonstd::expected<ZenEventData, ZenError> processEventData(ZenEventType eventType, gsl::span<const std::byte> data) noexcept = 0;

        virtual std::string_view type() const noexcept = 0;

        ISensorProperties* properties() noexcept { return m_properties.get(); }

    protected:
        std::unique_ptr<ISensorProperties> m_properties;
    };
}

#endif
