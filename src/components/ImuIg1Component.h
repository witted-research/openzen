#ifndef ZEN_COMPONENTS_IMUIG1COMPONENT_H_
#define ZEN_COMPONENTS_IMUIG1COMPONENT_H_

#include <atomic>

#include "SensorComponent.h"
#include "communication/SyncedModbusCommunicator.h"
#include "utility/Ownership.h"

#include "LpMatrix.h"

namespace zen
{
    class ImuIg1Component : public SensorComponent
    {
    public:
        ImuIg1Component(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& communicator, unsigned int version) noexcept;

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        ZenSensorInitError init() noexcept override;

        ZenError processData(uint8_t function, gsl::span<const std::byte> data) noexcept override;

        nonstd::expected<ZenEventData, ZenError> processEventData(ZenEvent_t eventType, gsl::span<const std::byte> data) noexcept override;

        std::string_view type() const noexcept override { return g_zenSensorType_Imu; }

    private:
        nonstd::expected<ZenEventData, ZenError> parseSensorData(gsl::span<const std::byte> data) const noexcept;

        // todo: use span for target array
        nonstd::expected<bool, ZenError> readVector3IfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const;

        nonstd::expected<bool, ZenError> readVector4IfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const;

        nonstd::expected<bool, ZenError> readScalarIfAvailable(ZenProperty_t checkProperty, gsl::span<const std::byte>& data, float * targetArray) const;

        struct IMUState
        {
            LpMatrix3x3f accAlignMatrix;
            LpMatrix3x3f gyrAlignMatrix;
            LpMatrix3x3f softIronMatrix;
            LpVector3f accBias;
            LpVector3f gyrBias;
            LpVector3f hardIronOffset;
            int32_t samplingRate;
        };
        mutable Owner<IMUState> m_cache;

        SyncedModbusCommunicator& m_communicator;
    };
}
#endif
