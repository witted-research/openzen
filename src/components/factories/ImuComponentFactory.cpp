#include "ImuComponentFactory.h"

#include "components/IMUComponent.h"

#include "InternalTypes.h"
#include "SensorProperties.h"
#include "properties/ImuPropertyRulesV1.h"
#include "properties/LegacyImuProperties.h"

namespace zen
{
    std::unique_ptr<ISensorProperties> make_properties(unsigned int version, uint8_t id, SyncedModbusCommunicator& communicator)
    {
        switch (version)
        {
        case 1:
            return std::make_unique<SensorProperties<ImuPropertyRulesV1>>(id, communicator);

        default:
            return nullptr;
        }
    }

    nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> ImuComponentFactory::make_component(
        unsigned int version,
        uint8_t id,
        SyncedModbusCommunicator& communicator
    ) const noexcept
    {
        // Legacy sensors require a "Command Mode" to active for accessing properties
        // as well a configuration bitset to determine which data to output
        if (version == 0)
        {
            auto properties = std::make_unique<LegacyImuProperties>(communicator);

            // Initialize to non-streaming
            if (ZenError_None != properties->setBool(ZenImuProperty_StreamData, false))
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);

            if (auto bitset = communicator.sendAndWaitForResult<uint32_t>(0u, static_cast<DeviceProperty_t>(EDevicePropertyInternal::Config), static_cast<ZenProperty_t>(EDevicePropertyInternal::Config), {}))
            {
                properties->setOutputDataBitset(*bitset);
                return std::make_unique<ImuComponent>(std::move(properties), communicator, version);
            }
            else
            {
                return nonstd::make_unexpected(ZenSensorInitError_RetrieveFailed);
            }
        }

        if (auto properties = make_properties(version, id, communicator))
            return std::make_unique<ImuComponent>(std::move(properties), communicator, version);

        return nonstd::make_unexpected(ZenSensorInitError_UnsupportedProtocol);
    }
}