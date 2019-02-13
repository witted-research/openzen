#ifndef ZEN_COMPONENTS_FACTORIES_IMUCOMPONENTFACTORY_H_
#define ZEN_COMPONENTS_FACTORIES_IMUCOMPONENTFACTORY_H_

#include "components/IComponentFactory.h"

namespace zen
{
    class ImuComponentFactory : public IComponentFactory
    {
    public:
        nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> make_component(
            uint8_t id,
            unsigned int version,
            AsyncIoInterface& ioInterface
        ) const noexcept override;
    };
}

#endif
