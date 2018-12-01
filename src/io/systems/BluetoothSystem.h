#ifndef ZEN_IO_SYSTEMS_BLUETOOTHSYSTEM_H_
#define ZEN_IO_SYSTEMS_BLUETOOTHSYSTEM_H_

#include "io/IIoSystem.h"

namespace zen
{
    class BluetoothSystem : public IIoSystem
    {
    public:
        constexpr static const char KEY[] = "Bluetooth";

        bool available() override;

        ZenError listDevices(std::vector<ZenSensorDesc>& outDevices) override;

        std::unique_ptr<BaseIoInterface> obtain(const ZenSensorDesc& desc, ZenError& outError) override;
    };
}

#endif
