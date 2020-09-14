//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/IoManager.h"

#include <new>
#include <type_traits>

#include "io/systems/BleSystem.h"
#include "io/systems/BluetoothSystem.h"
#include "io/systems/TestSensorSystem.h"
#ifdef ZEN_NETWORK
#include "io/systems/ZeroMQSystem.h"
#endif
#if WIN32
    #if ZEN_USE_BINARY_LIBRARIES
    #include "io/systems/PcanBasicSystem.h"
    #include "io/systems/FtdiUsbSystem.h"
    #include "io/systems/SiUsbSystem.h"
    #endif
#include "io/systems/windows/WindowsDeviceSystem.h"
#elif __linux__
#include "io/systems/linux/LinuxDeviceSystem.h"
#elif __APPLE__
#include "io/systems/mac/MacDeviceSystem.h"
#endif

namespace zen
{
    static auto testSensorRegistry = makeRegistry<TestSensorSystem>();
#ifdef ZEN_BLUETOOTH_BLE
    static auto bleRegistry = makeRegistry<BleSystem>();
#endif
#ifdef ZEN_BLUETOOTH
    static auto bluetoothRegistry = makeRegistry<BluetoothSystem>();
#endif
#ifdef ZEN_NETWORK
    static auto zmqRegistry = makeRegistry<ZeroMQSystem>();
#endif

#if WIN32
    #if ZEN_USE_BINARY_LIBRARIES
    static auto pcanRegistry = makeRegistry<PcanBasicSystem>();
    static auto siUsbRegistry = makeRegistry<SiUsbSystem>();
    static auto ftdiUsbRegistry = makeRegistry<FtdiUsbSystem>();
    #endif

    // [XXX] Need to re-evaluate the usage
    static auto windowsDeviceRegistry = makeRegistry<WindowsDeviceSystem>();
#elif __linux__
    static auto linuxDeviceRegistry = makeRegistry<LinuxDeviceSystem>();
#elif __APPLE__
    static auto macDeviceRegistry = makeRegistry<MacDeviceSystem>();
#endif

    IAutoIoSystemRegistry* IoManager::head = nullptr;
    IAutoIoSystemRegistry* IoManager::tail = nullptr;

    namespace IoManagerSingleton
    {
        static unsigned int g_niftyCounter = 0;
        static std::aligned_storage_t<sizeof(IoManager), alignof(IoManager)> g_singletonBuffer;
        IoManager& g_singleton = reinterpret_cast<IoManager&>(g_singletonBuffer);
    }

    IoManagerInitializer::IoManagerInitializer()
    {
        if (IoManagerSingleton::g_niftyCounter++ == 0)
            new (&IoManagerSingleton::g_singleton) IoManager();
    }

    IoManagerInitializer::~IoManagerInitializer()
    {
        if (--IoManagerSingleton::g_niftyCounter == 0)
            (&IoManagerSingleton::g_singleton)->~IoManager();
    }

    IoManager& IoManager::get() noexcept
    {
        return IoManagerSingleton::g_singleton;
    }

    void IoManager::initialize() noexcept
    {
        for (auto it = head; it != nullptr; it = it->next())
            it->initialize();
    }

    bool IoManager::registerIoSystem(std::string_view key, std::unique_ptr<IIoSystem> system) noexcept
    {
        if (!system->available())
            return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto result = m_ioSystems.emplace(key, std::move(system));
        return result.second;
    }

    std::optional<std::reference_wrapper<IIoSystem>> IoManager::getIoSystem(std::string_view key) const noexcept
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_ioSystems.find(key);
        if (it == m_ioSystems.end())
            return std::nullopt;

        return *it->second.get();
    }

    std::vector<std::reference_wrapper<IIoSystem>> IoManager::getIoSystems() const noexcept
    {
        std::vector<std::reference_wrapper<IIoSystem>> ioSystems;

        std::lock_guard<std::mutex> lock(m_mutex);
        ioSystems.reserve(m_ioSystems.size());

        for (const auto& pair : m_ioSystems)
            ioSystems.emplace_back(*pair.second.get());

        return ioSystems;
    }
}
