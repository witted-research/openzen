//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "io/systems/PcanBasicSystem.h"

#include <string>

#include "io/can/CanManager.h"
#include "io/interfaces/CanInterface.h"

#include "utility/IPlatformDll.h"

namespace zen
{
    PcanFnTable PcanBasicSystem::fnTable = {};

    PcanBasicSystem::~PcanBasicSystem()
    {
        // Invoke channel destructors first, because it uses dll functions
        m_channels.clear();

        IPlatformDll::get().unload(m_handle);
    }

    bool PcanBasicSystem::available()
    {
        if (m_handle != nullptr)
            return true;

        auto& dll = IPlatformDll::get();
        if (auto handle = dll.load("PCANBasic.dll"))
        {
            m_handle = handle;

            fnTable.getStatus = reinterpret_cast<PcanFnTable::GetStatusFn>(dll.procedure(m_handle, "CAN_GetStatus"));
            fnTable.initialize = reinterpret_cast<PcanFnTable::InitializeFn>(dll.procedure(m_handle, "CAN_Initialize"));
            fnTable.read = reinterpret_cast<PcanFnTable::ReadFn>(dll.procedure(m_handle, "CAN_Read"));
            fnTable.reset = reinterpret_cast<PcanFnTable::ResetFn>(dll.procedure(m_handle, "CAN_Reset"));
            fnTable.setValue = reinterpret_cast<PcanFnTable::SetValueFn>(dll.procedure(m_handle, "CAN_SetValue"));
            fnTable.uninitialize = reinterpret_cast<PcanFnTable::UninitializeFn>(dll.procedure(m_handle, "CAN_Uninitialize"));
            fnTable.write = reinterpret_cast<PcanFnTable::WriteFn>(dll.procedure(m_handle, "CAN_Write"));

            return openChannel(PCAN_USBBUS1) == ZenError_None;
        }

        return false;
    }

    ZenError PcanBasicSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        for (const auto& channel : m_channels)
            if (auto error = channel->listDevices(outDevices))
                return error;

        return ZenError_None;
    }

    nonstd::expected<std::unique_ptr<IIoInterface>, ZenSensorInitError> PcanBasicSystem::obtain(const ZenSensorDesc& desc, IIoDataSubscriber& subscriber) noexcept
    {
        ICanChannel& channel = *m_channels.begin()->get();

        char* end = const_cast<char*>(std::strchr(desc.identifier, '\0'));
        const auto deviceId = std::strtoul(desc.identifier, &end, 10);
        if (deviceId == std::numeric_limits<unsigned long>::max())
            return nonstd::make_unexpected(ZenSensorInitError_UnknownIdentifier);

        auto ioInterface = std::make_unique<CanInterface>(subscriber, channel, deviceId);
        if (!channel.subscribe(*ioInterface.get()))
            return nullptr;

        return ioInterface;
    }

    ZenError PcanBasicSystem::openChannel(TPCANHandle handle)
    {
        if (auto error = PcanBasicSystem::fnTable.initialize(handle, DEFAULT_BAUDRATE, 0, 0, 0))
            return ZenError_Io_InitFailed;

        auto channel = std::make_unique<PcanBasicChannel>(handle);

        unsigned char on = PCAN_PARAMETER_ON;
        if (auto result = PcanBasicSystem::fnTable.setValue(handle, PCAN_BUSOFF_AUTORESET, &on, sizeof(on)))
            return ZenError_Io_SetFailed;

        CanManager::get().registerChannel(*channel.get());
        m_channels.emplace(std::move(channel));
        return ZenError_None;
    }
}
