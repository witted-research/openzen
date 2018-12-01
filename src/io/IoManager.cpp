#include "io/IoManager.h"

#include <new>
#include <type_traits>

#include "io/systems/BleSystem.h"
#include "io/systems/BluetoothSystem.h"
#include "io/systems/FtdiUsbSystem.h"

#if WIN32
#include "io/systems/PcanBasicSystem.h"
#include "io/systems/SiUsbSystem.h"
#include "io/systems/windows/WindowsDeviceSystem.h"
#endif

namespace zen
{
    static auto bleRegistry = makeRegistry<BleSystem>();
    static auto bluetoothRegistry = makeRegistry<BluetoothSystem>();
    static auto ftdiUsbRegistry = makeRegistry<FtdiUsbSystem>();

#if WIN32
    static auto pcanRegistry = makeRegistry<PcanBasicSystem>();
    static auto siUsbRegistry = makeRegistry<SiUsbSystem>();

    // [XXX] Need to re-evaluate the usage
    //static auto windowsDeviceRegistry = makeRegistry<WindowsDeviceSystem>();
#endif

    IAutoIoSystemRegistry* IoManager::head = nullptr;
    IAutoIoSystemRegistry* IoManager::tail = nullptr;

    namespace
    {
        static unsigned int g_niftyCounter = 0;
        static std::aligned_storage_t<sizeof(IoManager), alignof(IoManager)> g_singletonBuffer;
        IoManager& g_singleton = reinterpret_cast<IoManager&>(g_singletonBuffer);
    }

    IoManagerInitializer::IoManagerInitializer()
    {
        if (g_niftyCounter++ == 0)
            new (&g_singleton) IoManager();
    }

    IoManagerInitializer::~IoManagerInitializer()
    {
        if (--g_niftyCounter == 0)
            (&g_singleton)->~IoManager();
    }

    IoManager& IoManager::get()
    {
        return g_singleton;
    }

    void IoManager::initialize()
    {
        for (auto it = head; it != nullptr; it = it->next())
            it->initialize();
    }

    void IoManager::deinitialize()
    {
        m_ioSystems.clear();
    }

    bool IoManager::registerIoSystem(std::string_view key, std::unique_ptr<IIoSystem> system)
    {
        if (!system->available())
            return false;

        auto result = m_ioSystems.emplace(key, std::move(system));
        return result.second;
    }

    std::unique_ptr<BaseIoInterface> IoManager::obtain(const ZenSensorDesc& desc, ZenError& outError)
    {
        auto it = m_ioSystems.find(desc.ioType);
        if (it == m_ioSystems.end())
        {
            outError = ZenError_Device_IoTypeInvalid;
            return nullptr;
        }

        return it->second->obtain(desc, outError);
    }

    ZenError IoManager::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        for (auto& pair : m_ioSystems)
            if (auto error = pair.second->listDevices(outDevices))
                return error;

        return ZenError_None;
    }
}
