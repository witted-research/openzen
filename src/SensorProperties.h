//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSORPROPERTIES_H_
#define ZEN_SENSORPROPERTIES_H_

#include "ISensorProperties.h"
#include "communication/SyncedModbusCommunicator.h"

namespace zen
{
    template <typename PropertyRules>
    class SensorProperties : public ISensorProperties
    {
    public:
        SensorProperties(uint8_t id, SyncedModbusCommunicator& communicator);


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
        nonstd::expected<uint64_t, ZenError> getUInt64(ZenProperty_t) noexcept override;

        /** If successful sets the array properties, otherwise returns an error. */
        ZenError setArray(ZenProperty_t property, ZenPropertyType type, gsl::span<const std::byte> buffer) noexcept override;

        /** If successful sets the boolean property, otherwise returns an error. */
        ZenError setBool(ZenProperty_t property, bool value) noexcept override;

        /** If successful sets the floating-point property, otherwise returns an error. */
        ZenError setFloat(ZenProperty_t property, float value) noexcept override;

        /** If successful sets the integer property, otherwise returns an error. */
        ZenError setInt32(ZenProperty_t property, int32_t value) noexcept override;

        /** If successful sets the unsigned integer property, otherwise returns an error. */
        ZenError setUInt64(ZenProperty_t property, uint64_t value) noexcept override;

        /** Returns whether the property is an array type */
        bool isArray(ZenProperty_t property) const noexcept override { return m_rules.isArray(property); }

        /** Returns whether the property is constant. If so, the property cannot be set */
        bool isConstant(ZenProperty_t property) const noexcept override { return m_rules.isConstant(property); }

        /** Returns whether the property can be executed as a command */
        bool isExecutable(ZenProperty_t property) const noexcept override { return m_rules.isExecutable(property); }

        /** Returns the type of the property */
        ZenPropertyType type(ZenProperty_t property) const noexcept override { return m_rules.type(property); }

    private:
        template <typename T>
        nonstd::expected<T, ZenError> getResult(ZenProperty_t property) noexcept;

        template <typename T>
        ZenError setAndAck(ZenProperty_t property, T value) noexcept;

        SyncedModbusCommunicator& m_communicator;
        PropertyRules m_rules;
        const uint8_t m_id;
    };

    namespace properties
    {
        ZenError publishAck(ISensorProperties& self, SyncedModbusCommunicator& communicator, ZenProperty_t property, ZenError error) noexcept;

        ZenError publishResult(ISensorProperties& self, SyncedModbusCommunicator& communicator, ZenProperty_t property, ZenError error, gsl::span<const std::byte> data) noexcept;
    }

    constexpr size_t sizeOfPropertyType(ZenPropertyType type) noexcept
    {
        switch (type)
        {
        case ZenPropertyType_Byte:
            return sizeof(std::byte);

        case ZenPropertyType_Bool:
            return sizeof(bool);

        case ZenPropertyType_Float:
            return sizeof(float);

        case ZenPropertyType_Int32:
            return sizeof(int32_t);

        case ZenPropertyType_UInt64:
            return sizeof(uint64_t);

        default:
            return 0;
        }
    }
}

#endif
