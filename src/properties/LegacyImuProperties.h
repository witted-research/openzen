//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_PROPERTIES_LEGACYIMUPROPERTIES_H_
#define ZEN_PROPERTIES_LEGACYIMUPROPERTIES_H_

#include <unordered_map>
#include <vector>

#include "communication/SyncedModbusCommunicator.h"
#include "components/ImuComponent.h"

namespace zen
{
    class LegacyImuProperties : public ISensorProperties
    {
    public:
        LegacyImuProperties(SyncedModbusCommunicator& communicator) noexcept;

        /** If successful executes the command, therwise returns an error. */
        ZenError execute(ZenProperty_t property) noexcept override;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        std::pair<ZenError, size_t> getArray(ZenProperty_t property, ZenPropertyType type, gsl::span<std::byte> buffer) noexcept override;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        nonstd::expected<bool, ZenError> getBool(ZenProperty_t property) noexcept override;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        nonstd::expected<float, ZenError> getFloat(ZenProperty_t property) noexcept override;

        /** If successful fills the value with the property's integer value, otherwise returns an error. */
        nonstd::expected<int32_t, ZenError> getInt32(ZenProperty_t property) noexcept override;

        /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
        nonstd::expected<uint64_t, ZenError> getUInt64(ZenProperty_t) noexcept override { return nonstd::make_unexpected(ZenError_UnknownProperty); }

        /** If successful sets the array properties, otherwise returns an error. */
        ZenError setArray(ZenProperty_t property, ZenPropertyType type, gsl::span<const std::byte> buffer) noexcept override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBool(ZenProperty_t property, bool value) noexcept override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        ZenError setFloat(ZenProperty_t property, float value) noexcept override;

        /** If successful sets the integer property, otherwise returns an error. */
        ZenError setInt32(ZenProperty_t property, int32_t value) noexcept override;

        /** If successful sets the unsigned integer property, otherwise returns an error. */
        ZenError setUInt64(ZenProperty_t, uint64_t) noexcept override { return ZenError_UnknownProperty; }

        /** Returns whether the property is an array type */
        bool isArray(ZenProperty_t property) const noexcept override;

        /** Returns whether the property can be executed as a command */
        bool isExecutable(ZenProperty_t property) const noexcept override;

        /** Returns whether the property is constant. If so, the property cannot be set */
        bool isConstant(ZenProperty_t property) const noexcept override;

        /** Returns the type of the property */
        ZenPropertyType type(ZenProperty_t property) const noexcept override;

        /** Manually initializes the config bitset from the sensor's config call. Will also
           initialize other variables like the gyro autocalibration setting. */
        void setConfigBitset(uint32_t bitset) noexcept;

    private:
        ZenError setPrecisionDataFlag(bool value) noexcept;

        struct IMUState
        {
            std::atomic_uint32_t samplingRate;
            std::atomic_uint32_t configBitset;
            std::atomic_bool gyrAutoCalibration;
        } m_cache;

        SyncedModbusCommunicator& m_communicator;

        std::atomic_bool m_streaming;
    };
}

#endif
