#include "components/GnssComponent.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <spdlog/spdlog.h>

#include "ImuHelpers.h"
///#include "GnssHelpers.h"
#include "SensorManager.h"
#include "properties/ImuSensorPropertiesV0.h"

#include <iostream>

namespace zen
{
    GnssComponent::GnssComponent(std::unique_ptr<ISensorProperties> properties, SyncedModbusCommunicator& com, unsigned int) noexcept
        : SensorComponent(std::move(properties)), m_communicator(com)
    {}

    ZenSensorInitError GnssComponent::init() noexcept
    {
        return ZenSensorInitError_None;
    }

    ZenError GnssComponent::processData(uint8_t /*function*/, gsl::span<const std::byte> /*data*/) noexcept
    {
        return ZenError_Io_UnsupportedFunction;
    }

    nonstd::expected<ZenEventData, ZenError> GnssComponent::processEventData(ZenEvent_t eventType, gsl::span<const std::byte> data) noexcept
    {
        switch (eventType)
        {
        case ZenGnssEvent_Sample:
            return parseSensorData(data);
            break;

        default:
            return nonstd::make_unexpected(ZenError_UnsupportedEvent);
        }
    }

    ZenError GnssComponent::storeGnssState() noexcept {
        if (ZenError_None != m_communicator.sendAndWaitForAck(0, uint8_t(EDevicePropertyV1::SaveGpsState),
            ZenProperty_t(EDevicePropertyV1::SaveGpsState),
            gsl::span<std::byte>()))
        {
            spdlog::error("Could not send command to Ig1 to presist data");

            return ZenError_Io_SendFailed;
        }
        spdlog::info("Command to presist Ig1 navigation data send.");

        return ZenError_None;
    }

    ZenError GnssComponent::close() noexcept {
        return storeGnssState();
    }

    nonstd::expected<ZenEventData, ZenError> GnssComponent::parseSensorData(gsl::span<const std::byte> /*data*/) const noexcept
    {
        // Any properties that are retrieved here should be cached locally, because it
        // will take too much time to retrieve from the sensor!
        ZenEventData eventData;

/*
        ZenEventData eventData;
        ZenGnssData& gnssData = eventData.gnssData;
        gnssDataReset(gnssData);

        const auto begin = data.begin();

        const auto size = data.size();
        const auto structSize = sizeof(Ig1RawGnss::IG1GpsData);
        if (size != structSize) {
            spdlog::warn("GNSS data packet size does not match with C struct");
            return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
        }
        if (std::distance(begin, data.begin() + structSize) != size) {
            std::cout << " data packages to small or too large for a GNSS packet" << std::endl;
            return nonstd::make_unexpected(ZenError_Io_MsgCorrupt);
        }
        Ig1RawGnss::IG1GpsData rawGnssData;
        // raw copy the content
        memcpy(&rawGnssData, data.data(), data.size());

        // convert
        gnssData.latitude = integerToScaledFloat(rawGnssData.pvt.latitude, -7);
        gnssData.longitude = integerToScaledFloat(rawGnssData.pvt.longitude, -7);

        gnssData.horizontalAccuracy = float(rawGnssData.pvt.hAcc);
        gnssData.verticalAccuracy = float(rawGnssData.pvt.vAcc);

        gnssData.height = float( rawGnssData.pvt.height );

        gnssData.heading = integerToScaledFloat(rawGnssData.pvt.headVeh, -5);
        gnssData.headingAccuracy = integerToScaledFloat(rawGnssData.pvt.headAcc, -5);

        gnssData.velocity = float(rawGnssData.pvt.gSpeed);
        gnssData.velocityAccuracy = float(rawGnssData.pvt.vAcc);
        gnssData.fixType = (ZenGnssFixType) rawGnssData.pvt.fixType;
        gnssData.numberSatelliteUsed = rawGnssData.pvt.numSV;
*/
        return eventData;
    }
}
