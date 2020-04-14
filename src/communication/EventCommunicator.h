//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMMUNICATION_EVENTCOMMUNICATOR_H_
#define ZEN_COMMUNICATION_EVENTCOMMUNICATOR_H_

#include "ZenTypes.h"

#include "io/IIoEventInterface.h"

namespace zen
{

    class IEventSubscriber
    {
    public:
        virtual ZenError processReceivedEvent(ZenEvent) noexcept = 0;
    };

    /*
    This class queries the connected sensor and tries to determines which type of sensor is connected
    and which components it provides. Currently, I implements a special case for the Ig1's two gyros
    and GNSS component.
    */
    class EventCommunicator : public IIoEventSubscriber {
    public:
        virtual ~EventCommunicator() = default;

        void init(std::unique_ptr<IIoEventInterface> ioInterface) noexcept
        {
            m_interface = std::move(ioInterface);
        }

        ZenError processEvent(ZenEvent ze) noexcept override {
           if (m_subscriber) {
                m_subscriber->processReceivedEvent(ze);
           }

           return ZenError_None;
        }

        /** Returns whether the IO interface equals the sensor description */
        bool equals(const ZenSensorDesc& desc) const noexcept { return m_interface->equals(desc); }

        void setSubscriber(IEventSubscriber& subscriber) noexcept { m_subscriber = &subscriber; }

        void close() {}

    protected:
        /** Publish received data to the subscriber */
        ZenError publishReceivedData(ZenEvent evt)  {
            return m_subscriber->processReceivedEvent(evt);
        }
        
    private:
        IEventSubscriber * m_subscriber = nullptr;         
        std::unique_ptr<IIoEventInterface> m_interface;
    };


}

#endif