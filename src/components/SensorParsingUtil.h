#ifndef ZEN_COMPONENTS_SENSORPARSING_UTIL_H_
#define ZEN_COMPONENTS_SENSORPARSING_UTIL_H_

#include "ZenTypes.h"
#include "ISensorProperties.h"

#include <gsl/span>
#include <cstring>
#include <cstddef>
#include <nonstd/expected.hpp>
#include <cmath>

namespace zen {
    namespace sensor_parsing_util {

        /**
        Parse a float32 froma byte stream and return the float
        */
        inline float parseFloat32(gsl::span<const std::byte>& data) noexcept
        {
            const int32_t temp = ((int32_t(data[3]) * 256 + int32_t(data[2])) * 256 + int32_t(data[1])) * 256 + int32_t(data[0]);
            data = data.subspan(sizeof(int32_t));
            float result;
            std::memcpy(&result, &temp, sizeof(int32_t));
            return result;
        }

        /**
        Convert an integer value to a float using a scale exponent according to this
        formula:

            float_out = int * 10^scaleExponent
        */
        template<class TIntegerType>
        inline double integerToScaledDouble(TIntegerType it, int32_t scaleExponent) {
            return double(it) * std::pow(double(10.0), double(scaleExponent));
        }

        inline nonstd::expected<bool, ZenError> readVector3IfAvailable(ZenProperty_t checkProperty,
            std::unique_ptr<ISensorProperties> & properties,
            gsl::span<const std::byte>& data, float * targetArray) {
            auto enabled = properties->getBool(checkProperty);
            if (!enabled)
                return enabled;

            if (*enabled) {
                for (unsigned idx = 0; idx < 3; ++idx)
                    targetArray[idx] = parseFloat32(data);
            }

            return enabled;
        }

        /**
        Templated function to read a scalar data type from a byte stream.
        */
        template <class TTypeToRead>
        inline void parseAndStoreScalar(gsl::span<const std::byte>& data, TTypeToRead *target);

        /**
        Specialization for uint32_t
        */
        template <>
        inline void parseAndStoreScalar(gsl::span<const std::byte>& data, uint32_t *target) {
            (*target) = *reinterpret_cast<const uint32_t*>(data.data());
            data = data.subspan(sizeof(uint32_t));
        }

        /**
        Specialization for uint16_t
        */
        template <>
        inline void parseAndStoreScalar(gsl::span<const std::byte>& data, uint16_t *target) {
            (*target) = *reinterpret_cast<const uint16_t*>(data.data());
            data = data.subspan(sizeof(uint16_t));
        }

        /**
        Specialization for uint8_t
        */
        template <>
        inline void parseAndStoreScalar(gsl::span<const std::byte>& data, uint8_t *target) {
            (*target) = *reinterpret_cast<const uint8_t*>(data.data());
            data = data.subspan(sizeof(uint8_t));
        }

        /**
        Specialization for int32_t
        */
        template <>
        inline void parseAndStoreScalar(gsl::span<const std::byte>& data, int32_t *target) {
            (*target) = *reinterpret_cast<const int32_t*>(data.data());
            data = data.subspan(sizeof(int32_t));
        }

        /**
        Templated function to check if the output property is enable and if so, read the
        data type from the span byte buffer.
        */
        template <class TTypeToRead>
        inline bool readScalarIfAvailable(ZenProperty_t checkProperty,
            std::unique_ptr<ISensorProperties> const& properties,
            gsl::span<const std::byte>& data, TTypeToRead * target) {
            auto enabled = properties->getBool(checkProperty);
            if (!enabled)
                return false;

            if (*enabled) {
                parseAndStoreScalar<TTypeToRead>(data, target);
                return true;
            }

            return false;
        }
    }
}

#endif
