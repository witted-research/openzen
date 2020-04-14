//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "SensorProperties.h"

#include <cstring>

#include "ZenProtocol.h"

#include "properties/CorePropertyRulesV1.h"
#include "properties/ImuPropertyRulesV1.h"

namespace zen
{
    namespace details
    {
        template <typename T>
        struct PropertyType
        {};

        template <> struct PropertyType<bool>
        {
            using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Bool>;
        };

        template <> struct PropertyType<float>
        {
            using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Float>;
        };

        template <> struct PropertyType<int32_t>
        {
            using type = std::integral_constant<ZenPropertyType, ZenPropertyType_Int32>;
        };

        template <> struct PropertyType<uint64_t>
        {
            using type = std::integral_constant<ZenPropertyType, ZenPropertyType_UInt64>;
        };

        class PropertyData
        {
        public:
            template <typename T>
            PropertyData(ZenProperty_t property, T value) noexcept
                : m_buffer(sizeof(property) + sizeof(value))
            {
                auto dst = m_buffer.data();
                std::memcpy(dst, &property, sizeof(property));
                std::memcpy(dst + sizeof(property), &value, sizeof(value));

            }

            PropertyData(ZenProperty_t property, const void* buffer, size_t bufferSize) noexcept
                : m_buffer(sizeof(property) + bufferSize)
            {
                auto dst = m_buffer.data();
                std::memcpy(dst, &property, sizeof(property));
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
                std::memcpy(dst + sizeof(property), buffer, bufferSize);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
            }

            gsl::span<const std::byte> data() const noexcept { return m_buffer; }

        private:
            std::vector<std::byte> m_buffer;
        };
    }

    template <typename PropertyRules>
    SensorProperties<PropertyRules>::SensorProperties(uint8_t id, SyncedModbusCommunicator& communicator)
        : m_communicator(communicator)
        , m_id(id)
    {}

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::execute(ZenProperty_t property) noexcept
    {
        if (m_rules.isExecutable(property))
        {
            const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
            return m_communicator.sendAndWaitForAck(m_id, ZenProtocolFunction_Execute, property, span);
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    std::pair<ZenError, size_t> SensorProperties<PropertyRules>::getArray(ZenProperty_t property, ZenPropertyType type, gsl::span<std::byte> buffer) noexcept
    {
        if (m_rules.isArray(property) && m_rules.type(property) == type)
        {
            switch (type)
            {
            case ZenPropertyType_Byte:
            {
                const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
                return m_communicator.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, buffer);
            }

            case ZenPropertyType_Bool:
            {
                const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
                return m_communicator.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, gsl::make_span(reinterpret_cast<bool*>(buffer.data()), buffer.size()));
            }

            case ZenPropertyType_Float:
            {
                const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
                return m_communicator.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, gsl::make_span(reinterpret_cast<float*>(buffer.data()), buffer.size()));
            }

            case ZenPropertyType_Int32:
            {
                const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
                return m_communicator.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, gsl::make_span(reinterpret_cast<int32_t*>(buffer.data()), buffer.size()));
            }

            case ZenPropertyType_UInt64:
            {
                const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
                return m_communicator.sendAndWaitForArray(m_id, ZenProtocolFunction_Get, property, span, gsl::make_span(reinterpret_cast<uint64_t*>(buffer.data()), buffer.size()));
            }

            default:
                return std::make_pair(ZenError_WrongDataType, buffer.size());
            }
        }

        return std::make_pair(ZenError_UnknownProperty, buffer.size());
    }

    template <typename PropertyRules>
    nonstd::expected<bool, ZenError> SensorProperties<PropertyRules>::getBool(ZenProperty_t property) noexcept
    {
        return getResult<bool>(property);
    }

    template <typename PropertyRules>
    nonstd::expected<float, ZenError> SensorProperties<PropertyRules>::getFloat(ZenProperty_t property) noexcept
    {
        return getResult<float>(property);
    }

    template <typename PropertyRules>
    nonstd::expected<int32_t, ZenError> SensorProperties<PropertyRules>::getInt32(ZenProperty_t property) noexcept
    {
        return getResult<int32_t>(property);
    }

    template <typename PropertyRules>
    nonstd::expected<uint64_t, ZenError> SensorProperties<PropertyRules>::getUInt64(ZenProperty_t property) noexcept
    {
        return getResult<uint64_t>(property);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setArray(ZenProperty_t property, ZenPropertyType type, gsl::span<const std::byte> buffer) noexcept
    {
        if (m_rules.isArray(property) && !m_rules.isConstant(property) && m_rules.type(property) == type)
        {
            const details::PropertyData wrapper(property, buffer.data(), sizeOfPropertyType(type) * buffer.size());
            if (auto error = m_communicator.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data()))
                return error;

            notifyPropertyChange(property, buffer);
            return ZenError_None;
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setBool(ZenProperty_t property, bool value) noexcept
    {
        return setAndAck<bool>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setFloat(ZenProperty_t property, float value) noexcept
    {
        return setAndAck<float>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setInt32(ZenProperty_t property, int32_t value) noexcept
    {
        return setAndAck<int32_t>(property, value);
    }

    template <typename PropertyRules>
    ZenError SensorProperties<PropertyRules>::setUInt64(ZenProperty_t property, uint64_t value) noexcept
    {
        return setAndAck<uint64_t>(property, value);
    }

    template <typename PropertyRules>
    template <typename T>
    ZenError SensorProperties<PropertyRules>::setAndAck(ZenProperty_t property, T value) noexcept
    {
        if (!m_rules.isConstant(property) && !m_rules.isArray(property) && m_rules.type(property) == details::PropertyType<T>::type::value)
        {
            const details::PropertyData wrapper(property, value);
            if (auto error = m_communicator.sendAndWaitForAck(m_id, ZenProtocolFunction_Set, property, wrapper.data()))
                return error;

            notifyPropertyChange(property, value);
            return ZenError_None;
        }

        return ZenError_UnknownProperty;
    }

    template <typename PropertyRules>
    template <typename T>
    nonstd::expected<T, ZenError> SensorProperties<PropertyRules>::getResult(ZenProperty_t property) noexcept
    {
        if (!m_rules.isArray(property) && m_rules.type(property) == details::PropertyType<T>::type::value)
        {
            const auto span = gsl::make_span(reinterpret_cast<const std::byte*>(&property), sizeof(property));
            return m_communicator.sendAndWaitForResult<T>(m_id, ZenProtocolFunction_Get, property, span);
        }

        return nonstd::make_unexpected(ZenError_UnknownProperty);
    }

    template class SensorProperties<CorePropertyRulesV1>;
    template class SensorProperties<ImuPropertyRulesV1>;

    namespace properties
    {
        ZenError publishAck(ISensorProperties& self, SyncedModbusCommunicator& communicator, ZenProperty_t property, ZenError error) noexcept
        {
            if (self.isExecutable(property) || (!self.isConstant(property) && self.type(property) != ZenPropertyType_Invalid))
                return communicator.publishAck(property, error);

            return ZenError_InvalidArgument;
        }

        ZenError publishResult(ISensorProperties& self, SyncedModbusCommunicator& communicator, ZenProperty_t property, ZenError error, gsl::span<const std::byte> data) noexcept
        {
            if (auto type = self.type(property))
            {
                if (self.isArray(property))
                {
                    switch (type)
                    {
                    case ZenPropertyType_Byte:
                        return communicator.publishArray(property, error, data);

                    case ZenPropertyType_Bool:
                        return communicator.publishArray(property, error, gsl::make_span(reinterpret_cast<const bool*>(data.data()), data.size() / sizeof(bool)));

                    case ZenPropertyType_Float:
                        return communicator.publishArray(property, error, gsl::make_span(reinterpret_cast<const float*>(data.data()), data.size() / sizeof(float)));

                    case ZenPropertyType_Int32:
                        return communicator.publishArray(property, error, gsl::make_span(reinterpret_cast<const int32_t*>(data.data()), data.size() / sizeof(int32_t)));

                    case ZenPropertyType_UInt64:
                        return communicator.publishArray(property, error, gsl::make_span(reinterpret_cast<const uint64_t*>(data.data()), data.size() / sizeof(uint64_t)));

                    default:
                        return ZenError_InvalidArgument;
                    }
                }
                else
                {
                    if (static_cast<size_t>(data.size()) != sizeOfPropertyType(type))
                        return ZenError_Io_MsgCorrupt;

                    switch (type)
                    {
                    case ZenPropertyType_Bool:
                        return communicator.publishResult(property, error, *reinterpret_cast<const bool*>(data.data()));

                    case ZenPropertyType_Float:
                        return communicator.publishResult(property, error, *reinterpret_cast<const float*>(data.data()));

                    case ZenPropertyType_Int32:
                        return communicator.publishResult(property, error, *reinterpret_cast<const int32_t*>(data.data()));

                    case ZenPropertyType_UInt64:
                        return communicator.publishResult(property, error, *reinterpret_cast<const uint64_t*>(data.data()));

                    default:
                        return ZenError_InvalidArgument;
                    }
                }
            }

            return ZenError_InvalidArgument;
        }
    }
}
