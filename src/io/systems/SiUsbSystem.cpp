#include "io/systems/SiUsbSystem.h"

#include <iostream>

#include "io/interfaces/SiUsbInterface.h"
#include "utility/IPlatformDll.h"

namespace zen
{
    SiUsbFnTable SiUsbSystem::fnTable = {};

    SiUsbSystem::~SiUsbSystem()
    {
        IPlatformDll::get().unload(m_handle);
    }

    bool SiUsbSystem::available()
    {
        if (m_handle != nullptr)
            return true;

        auto& dll = IPlatformDll::get();
        if (auto handle = dll.load("SiUSBXp.dll"))
        {
            m_handle = handle;

            fnTable.checkRxQueue = reinterpret_cast<SiUsbFnTable::CheckRxQueueFn>(dll.procedure(m_handle, "SI_CheckRXQueue"));
            fnTable.close = reinterpret_cast<SiUsbFnTable::CloseFn>(dll.procedure(m_handle, "SI_Close"));
            fnTable.getDeviceProductString = reinterpret_cast<SiUsbFnTable::GetDeviceProductStringFn>(dll.procedure(m_handle, "SI_GetDeviceProductString"));
            fnTable.getNumDevices = reinterpret_cast<SiUsbFnTable::GetNumDevicesFn>(dll.procedure(m_handle, "SI_GetNumDevices"));
            fnTable.getProductStringSafe = reinterpret_cast<SiUsbFnTable::GetProductStringSafeFn>(dll.procedure(m_handle, "SI_GetProductStringSafe"));
            fnTable.open = reinterpret_cast<SiUsbFnTable::OpenFn>(dll.procedure(m_handle, "SI_Open"));
            fnTable.read = reinterpret_cast<SiUsbFnTable::ReadFn>(dll.procedure(m_handle, "SI_Read"));
            fnTable.setBaudrate = reinterpret_cast<SiUsbFnTable::SetBaudrateFn>(dll.procedure(m_handle, "SI_SetBaudRate"));
            fnTable.setFlowControl = reinterpret_cast<SiUsbFnTable::SetFlowControlFn>(dll.procedure(m_handle, "SI_SetFlowControl"));
            fnTable.write = reinterpret_cast<SiUsbFnTable::WriteFn>(dll.procedure(m_handle, "SI_Write"));
            std::cout << "Loaded SiLabs driver." << std::endl;
            return true;
        }

        return false;
    }

    ZenError SiUsbSystem::listDevices(std::vector<ZenSensorDesc>& outDevices)
    {
        DWORD nDevices;
        if (auto error = SiUsbSystem::fnTable.getNumDevices(&nDevices))
            return ZenError_Io_GetFailed;

        ZenSensorDesc desc;
        std::memcpy(desc.ioType, SiUsbSystem::KEY, sizeof(SiUsbSystem::KEY));

        for (DWORD idx = 0; idx < nDevices; ++idx)
        {
            if (auto error = SiUsbSystem::fnTable.getProductStringSafe(idx, desc.serialNumber, sizeof(ZenSensorDesc::serialNumber), SI_RETURN_SERIAL_NUMBER))
                return ZenError_Io_GetFailed;

            if (auto error = SiUsbSystem::fnTable.getProductStringSafe(idx, desc.name, sizeof(ZenSensorDesc::name), SI_RETURN_DESCRIPTION))
                return ZenError_Io_GetFailed;

            desc.handle32 = idx;
            outDevices.emplace_back(desc);
        }

        return ZenError_None;
    }

    std::unique_ptr<BaseIoInterface> SiUsbSystem::obtain(const ZenSensorDesc& desc, ZenError& outError)
    {
        // Calling SI_GetNumDevices is required before calling SI_Open
        DWORD nDevices;
        if (auto error = SiUsbSystem::fnTable.getNumDevices(&nDevices))
        {
            outError = ZenError_Io_GetFailed;
            return nullptr;
        }

        if (desc.handle32 >= nDevices)
        {
            outError = ZenError_UnknownDeviceId;
            return nullptr;
        }

        HANDLE handle;
        if (auto error = fnTable.open(static_cast<DWORD>(desc.handle32), &handle))
        {
            outError = ZenError_Io_InitFailed;
            return nullptr;
        }

        auto format = modbus::ModbusFormat::LP;
        auto factory = modbus::make_factory(format);
        auto parser = modbus::make_parser(format);
        if (!factory || !parser)
        {
            outError = ZenError_InvalidArgument;
            return nullptr;
        }

        auto ioInterface = std::make_unique<SiUsbInterface>(handle, std::move(factory), std::move(parser));
        if (outError = ioInterface->setBaudrate(921600))
            return nullptr;

        if (auto error = fnTable.setFlowControl(handle, SI_HANDSHAKE_LINE, SI_FIRMWARE_CONTROLLED, SI_HELD_INACTIVE, SI_STATUS_INPUT, SI_STATUS_INPUT, 0))
        {
            outError = ZenError_Io_SetFailed;
            return nullptr;
        }

        return ioInterface;
    }
}
