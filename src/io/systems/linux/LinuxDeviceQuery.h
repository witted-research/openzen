//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef LINUX_DEVICE_QUERY_H
#define LINUX_DEVICE_QUERY_H

#include "utility/StringView.h"

#include <spdlog/spdlog.h>

/*
Older GCCs (for examle GCC 7, default on Ubunut 18.04) and clangs still provide
C++ filesystem in an experimental namespace
*/
#if __GNUC__ > 8
    #include <filesystem>
    namespace fs = std::filesystem;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#endif

#include <map>
#include <vector>
#include <fstream>
#include <string>
#include <optional>

namespace LinuxDeviceQuery
{

typedef std::map<std::string, std::vector<std::string>> SiLabsSerialDevices;

/**
 * Returns the contents of a device in the sysfs tree
 */
std::optional<std::string> sysFsGetDeviceProperty(fs::path const& devicePath,
    std::string const& propertyName) {
    std::ifstream propFile;

    propFile.open(devicePath / propertyName, std::ifstream::in);
    if (!propFile) {
        return std::nullopt;
    }

    std::string propLine;
    std::getline(propFile, propLine);
    propFile.close();

    return propLine;
}

/**
 * Gets the topmost folder of a sysfs usb device and traverses it to find the name of
 * the tty device assicated.
 */
std::optional<fs::path> sysFsGetDeviceTtyPath(fs::path const& devicePath) {
    for (auto &p : fs::directory_iterator(devicePath))
    {
        // p is something like /sys/bus/usb/devices/1-6/1-6:1.0
        if (util::endsWith(p.path(), ":1.0"))
        {
            // found device folder, look for tty* Folder inside
            for (auto &deviceFolder : fs::directory_iterator(p.path()))
            {
                auto lastFolder = deviceFolder.path().end();
                if (deviceFolder.path().begin() != lastFolder)
                {
                    lastFolder = --lastFolder;
                    if (util::startsWith(*lastFolder, "tty"))
                    {
                        // generate the file name on the system /dev folder
                        const auto deviceFileName = fs::path("/dev/") / std::string(*lastFolder);
                        return deviceFileName;
                    }
                }
            }
        }
    }

    return std::nullopt;
}

/**
 * Returns true if a sysfs USB device is a SiLabs CP210x UART interface chip
 */
bool isSiLabsDevice(std::string const& devicePath ) {
    auto vendor = sysFsGetDeviceProperty(devicePath, "idVendor");
    auto product = sysFsGetDeviceProperty(devicePath, "idProduct");

    if (!vendor)
        return false;
    if (!product)
        return false;

    // Check for SiLabs
    if (*vendor != "10c4")
    {
        return false;
    }

    // Check for CP210x USB UART bridge product ids
    if ((*product != "ea61") && (*product != "ea60"))
    {
        return false;
    }

    return true;
}

/**
 * Returns a list of all serial strings found of SiLabs UART chips
 * and the serial devices (like "/dev/ttyUSB0") assicated with them.
 * One serial string can in principle appear on multiple devices.
 */
SiLabsSerialDevices getSiLabsDevices()
{
    SiLabsSerialDevices found_devices;
    const std::string sysfs_usb_path = "/sys/bus/usb/devices/";
    // list all connected usb devices
    for (auto &usb_device : fs::directory_iterator(sysfs_usb_path))
    {
        // check if SiLabs Device
        if (!isSiLabsDevice(usb_device.path()))
            continue;

        // get serial string
        auto serial_string = sysFsGetDeviceProperty(usb_device.path(), "serial");

        if (!serial_string)
            continue;

        const auto ttyDevice = sysFsGetDeviceTtyPath(usb_device.path());
        if (ttyDevice) {
            spdlog::info("Found serial name {0} for device {1}", *serial_string, ttyDevice->u8string());
            found_devices[*serial_string].push_back(*ttyDevice);
        }
    }

    return found_devices;
}

/** Returns all serial devices ("/dev/ttyUSB") assicated with one
 * specific serial string of SiLabs chip.
 * One serial number can in principle appear on multiple devices.
*/
std::vector<std::string> getDeviceFileForSiLabsSerial(std::string const &serial_string)
{
    auto devices = getSiLabsDevices();

    #if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    for (auto const &dev : devices)
    {
        SPDLOG_DEBUG("Serial Number {0} found for these devices", dev.first);
        for (auto const &devFileName : dev.second)
        {
            SPDLOG_DEBUG("-> Device file name {0}", devFileName);
        }
    }
    #endif

    if (devices.find(serial_string) == devices.end())
    {
        spdlog::error("No SiLabs USB device with serial string {0} found on system", serial_string);
        return std::vector<std::string>();
    }

    return devices[serial_string];
}

} // namespace LinuxDevices

#endif
