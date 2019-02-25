#ifndef ZEN_COMPONENTS_ICOMPONENTFACTORY_H_
#define ZEN_COMPONENTS_ICOMPONENTFACTORY_H_

#include <memory>
#include <string_view>

#include "nonstd/expected.hpp"

#include "SensorComponent.h"

namespace zen
{
    class IComponentFactory
    {
    public:
        virtual ~IComponentFactory() = default;

        virtual nonstd::expected<std::unique_ptr<SensorComponent>, ZenSensorInitError> make_component(
            unsigned int version,
            uint8_t id,
            class SyncedModbusCommunicator& communicator
        ) const noexcept = 0;
    };
}

#endif
