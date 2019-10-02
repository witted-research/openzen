#ifndef ZEN_COMPONENTS_FACTORIES_GNSSCOMPONENTFACTORY_H_
#define ZEN_COMPONENTS_FACTORIES_GNSSCOMPONENTFACTORY_H_

#include "components/IComponentFactory.h"

namespace zen
{
    class GnssComponentFactory : public IComponentFactory
    {
    public:
        nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> make_component(
            unsigned int version,
            uint8_t id,
            SyncedModbusCommunicator& communicator
        ) const noexcept override;
    };
}

#endif
