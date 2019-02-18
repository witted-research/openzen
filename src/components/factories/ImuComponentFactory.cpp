#include "ImuComponentFactory.h"

#include "components/IMUComponent.h"

#include "InternalTypes.h"
#include "SensorProperties.h"
#include "properties/ImuPropertyRulesV1.h"
#include "properties/LegacyImuProperties.h"

namespace zen
{
    std::unique_ptr<ISensorProperties> make_properties(uint8_t id, unsigned int version, AsyncIoInterface& ioInterface)
    {
        switch (version)
        {
        case 1:
            return std::make_unique<SensorProperties<ImuPropertyRulesV1>>(id, ioInterface);

        default:
            return nullptr;
        }
    }

    nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> ImuComponentFactory::make_component(
        uint8_t id,
        unsigned int version,
        AsyncIoInterface& ioInterface
    ) const noexcept
    {
        // Legacy sensors require a "Command Mode" to active for accessing properties
        // as well a configuration bitset to determine which data to output
        if (version == 0)
        {
            auto properties = std::make_unique<LegacyImuProperties>(ioInterface);

            // Initialize to non-streaming
            if (ZenError_None != properties->setBool(ZenImuProperty_StreamData, false))
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);

            uint32_t newBitset;
            if (ZenError_None != ioInterface.sendAndWaitForResult(0, static_cast<DeviceProperty_t>(EDevicePropertyInternal::Config), static_cast<ZenProperty_t>(EDevicePropertyInternal::Config), {}, newBitset))
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);

            properties->setOutputDataBitset(newBitset);
            return std::make_unique<ImuComponent>(version, std::move(properties), ioInterface);
        }

        if (auto properties = make_properties(id, version, ioInterface))
            return std::make_unique<ImuComponent>(version, std::move(properties), ioInterface);

        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedProtocol);
    }
}