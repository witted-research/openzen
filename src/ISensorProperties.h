#ifndef ZEN_ISENSORPROPERTIES_H_
#define ZEN_ISENSORPROPERTIES_H_

#include <cstdint>
#include <cstddef>
#include <functional>
#include <utility>
#include <variant>

#include <gsl/span>
#include <nonstd/expected.hpp>

#include "ZenTypes.h"

namespace zen
{
    using SensorPropertyValue = std::variant<
        bool,
        float,
        int32_t,
        uint64_t,
        gsl::span<const std::byte>
    >;

    using SensorPropertyChangeCallback = std::function<void(SensorPropertyValue)>;

    class ISensorProperties
    {
    public:
        virtual ~ISensorProperties() = default;

        /** If successful executes the property, otherwise returns an error. */
        virtual ZenError execute(ZenProperty_t property) noexcept = 0;

        /** If successful fills the buffer with the array of properties and sets the buffer's size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual std::pair<ZenError, size_t> getArray(ZenProperty_t property, ZenPropertyType type, gsl::span<std::byte> buffer) noexcept = 0;

        /** If successful fills the value with the property's boolean value, otherwise returns an error. */
        virtual nonstd::expected<bool, ZenError> getBool(ZenProperty_t property) noexcept = 0;

        /** If successful fills the value with the property's floating-point value, otherwise returns an error. */
        virtual nonstd::expected<float, ZenError> getFloat(ZenProperty_t property) noexcept = 0;

        /** If successful fills the value with the property's signed integer value, otherwise returns an error. */
        virtual nonstd::expected<int32_t, ZenError> getInt32(ZenProperty_t property) noexcept = 0;

        /** If successful fills the buffer with the property's string value and sets the buffer's string size.
         * Otherwise, returns an error and potentially sets the desired buffer size - if it is too small.
         */
        virtual std::pair<ZenError, size_t> getString(ZenProperty_t property, gsl::span<char> buffer) noexcept = 0;

        /** If successful fills the value with the property's unsigned integer value, otherwise returns an error. */
        virtual nonstd::expected<uint64_t, ZenError> getUInt64(ZenProperty_t property) noexcept = 0;

        /** If successful sets the array properties, otherwise returns an error. */
        virtual ZenError setArray(ZenProperty_t property, ZenPropertyType type, gsl::span<const std::byte> buffer) noexcept = 0;

        /** If successful sets the boolean property, otherwise returns an error. */
        virtual ZenError setBool(ZenProperty_t property, bool value) noexcept = 0;

        /** If successful sets the floating-point property, otherwise returns an error. */
        virtual ZenError setFloat(ZenProperty_t property, float value) noexcept = 0;

        /** If successful sets the integer property, otherwise returns an error. */
        virtual ZenError setInt32(ZenProperty_t property, int32_t value) noexcept = 0;

        /** If successful sets the string property, otherwise returns an error. */
        virtual ZenError setString(ZenProperty_t property, gsl::span<const char> buffer) noexcept = 0;

        /** If successful sets the unsigned integer property, otherwise returns an error. */
        virtual ZenError setUInt64(ZenProperty_t property, uint64_t value) noexcept = 0;

        /** Returns whether the property is an array type */
        virtual bool isArray(ZenProperty_t property) const noexcept = 0;

        /** Returns whether the property is constant. If so, the property cannot be set */
        virtual bool isConstant(ZenProperty_t property) const noexcept = 0;

        /** Returns whether the property can be executed as a command */
        virtual bool isExecutable(ZenProperty_t property) const noexcept = 0;

        /** Returns the type of the property */
        virtual ZenPropertyType type(ZenProperty_t property) const noexcept = 0;

        /** Subscribes to change notifications of the property */
        void subscribeToPropertyChanges(ZenProperty_t property, SensorPropertyChangeCallback callback) noexcept;

    protected:
        /** Trigger  */
        void notifyPropertyChange(ZenProperty_t property, SensorPropertyValue value) const noexcept;

    private:
        std::unordered_map<ZenProperty_t, std::vector<SensorPropertyChangeCallback>> m_subscriberCallbacks;
    };
}

#endif
