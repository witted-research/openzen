//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMPONENTS_IMUCOMPONENT_H_
#define ZEN_COMPONENTS_IMUCOMPONENT_H_

#include <atomic>

#include "SensorComponent.h"
#include "communication/SyncedModbusCommunicator.h"
#include "utility/Ownership.h"

#include "LpMatrix.h"

namespace zen
{
    class ImuComponent : public SensorComponent
    {
    public:
        ImuComponent(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& communicator, unsigned int version) noexcept;

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        ZenSensorInitError init() noexcept override;

        ZenError processData(uint8_t function, gsl::span<const std::byte> data) noexcept override;

        nonstd::expected<ZenEventData, ZenError> processEventData(ZenEventType eventType, gsl::span<const std::byte> data) noexcept override;

        std::string_view type() const noexcept override { return g_zenSensorType_Imu; }

    private:
        nonstd::expected<ZenEventData, ZenError> parseSensorData(gsl::span<const std::byte> data) const noexcept;

        struct IMUState
        {
            LpMatrix3x3f accAlignMatrix;
            LpMatrix3x3f gyrAlignMatrix;
            LpMatrix3x3f softIronMatrix;
            LpVector3f accBias;
            LpVector3f gyrBias;
            LpVector3f hardIronOffset;
        };
        mutable Owner<IMUState> m_cache;

        SyncedModbusCommunicator& m_communicator;
        
        const unsigned int m_version;
    };
}
#endif
