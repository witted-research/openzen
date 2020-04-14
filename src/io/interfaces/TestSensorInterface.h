//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_IO_INTERFACES_TESTSENSORINTERFACE_H_
#define ZEN_IO_INTERFACES_TESTSENSORINTERFACE_H_

#include <array>
#include <atomic>
#include <thread>

#include "io/IIoEventInterface.h"

namespace zen
{
    class TestSensorInterface : public IIoEventInterface
    {
    public:
        TestSensorInterface(IIoEventSubscriber& subscriber, std::string const& endpoint) noexcept;
        ~TestSensorInterface();

        /** Returns the type of IO interface */
        std::string_view type() const noexcept override;

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept override;

    private:
        int run();

        std::atomic_bool m_terminate;
        std::thread m_pollingThread;
    };
}

#endif
