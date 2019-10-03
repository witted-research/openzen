#ifndef ZEN_COMPONENTS_GNSSCOMPONENT_H_
#define ZEN_COMPONENTS_GNSSCOMPONENT_H_

#include <atomic>
#include <chrono>
#include <optional>

#include "SensorComponent.h"
#include "communication/SyncedModbusCommunicator.h"
#include "utility/Ownership.h"

#include "LpMatrix.h"

namespace zen
{
    /**
    This component parses the GNSS data provided by an Ig1 sensor
    */
    class GnssComponent : public SensorComponent
    {
    public:
        GnssComponent(std::unique_ptr<ISensorProperties> properties,
            SyncedModbusCommunicator& communicator, unsigned int version) noexcept;

        /** Tries to initialize settings of the sensor's component that can fail.
         * After succesfully completing init, m_properties should be set.
         */
        ZenSensorInitError init() noexcept override;

        ZenError close() noexcept override;

        ZenError processData(uint8_t function, gsl::span<const std::byte> data) noexcept override;

        nonstd::expected<ZenEventData, ZenError> processEventData(ZenEvent_t eventType,
            gsl::span<const std::byte> data) noexcept override;

        std::string_view type() const noexcept override { return g_zenSensorType_Gnss; }

    private:
        ZenError storeGnssState() noexcept;
        nonstd::expected<ZenEventData, ZenError> parseSensorData(gsl::span<const std::byte> data) const noexcept;
        SyncedModbusCommunicator & m_communicator;
    };
}
#endif
