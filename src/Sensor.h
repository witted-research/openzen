//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_SENSOR_H_
#define ZEN_SENSOR_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "nonstd/expected.hpp"

#include "InternalTypes.h"

#include "SensorConfig.h"
#include "SensorComponent.h"
#include "communication/SyncedModbusCommunicator.h"
#include "communication/EventCommunicator.h"
#include "utility/LockingQueue.h"
#include "utility/ReferenceCmp.h"
#include "processors/DataProcessor.h"


namespace zen
{
    nonstd::expected<std::shared_ptr<class Sensor>, ZenSensorInitError> make_sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token) noexcept;

    nonstd::expected<std::shared_ptr<class Sensor>, ZenSensorInitError> make_high_level_sensor(SensorConfig config, std::unique_ptr<EventCommunicator> evCom, uintptr_t token) noexcept;

    class Sensor final : private IModbusFrameSubscriber, IEventSubscriber
    {
    public:
        Sensor(SensorConfig config, std::unique_ptr<ModbusCommunicator> communicator, uintptr_t token);

        /**
        creates a high-level sensor which receives events directly and needs to parsing
        */
        Sensor(SensorConfig config, std::unique_ptr<EventCommunicator> eventCommunicator, uintptr_t token);

        ~Sensor();

        /** Allow the sensor to initialize variables, that require an active IO interface */
        ZenSensorInitError init();

        /** On first call, tries to initialize a firmware update, and returns an error on failure.
         * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
         * Returns ZenAsync_Updating while busy updating firmware.
         * Returns ZenAsync_Finished once the entire firmware has been written to the sensor.
         * Returns ZenAsync_Failed if an error has occured while updating.
         */
        ZenAsyncStatus updateFirmwareAsync(gsl::span<const std::byte> buffer) noexcept;

        /** On first call, tries to initialize an IAP update, and returns an error on failure.
         * Subsequent calls do not require a valid buffer and buffer size, and only report the current status:
         * Returns ZenAsync_Updating while busy updating IAP.
         * Returns ZenAsync_Finished once the entire IAP has been written to the sensor.
         * Returns ZenAsync_Failed if an error has occured while updating.
         */
        ZenAsyncStatus updateIAPAsync(gsl::span<const std::byte> buffer) noexcept;

        /** Returns an interface for the sensor's properties */
        ISensorProperties* properties() { return m_properties.get(); }

        /** If successful, directs the outComponents pointer to a list of sensor components and sets its length to outLength, otherwise, returns an error.
         * If the type variable points to a string, only components of that type are returned. If it is a nullptr, all components are returned, irrespective of type.
         */
        const std::vector<std::unique_ptr<SensorComponent>>& components() const noexcept { return m_components; }

        /** Returns the sensor's IO type */
        std::string_view ioType() const noexcept { return m_communicator->ioType(); }

        /** Returns whether the sensor is equal to the sensor description */
        bool equals(const ZenSensorDesc& desc) const;

        /** Returns the sensor's unique token */
        uintptr_t token() const noexcept { return m_token; }

        /** Subscribe an event queue to the sensor */
        bool subscribe(LockingQueue<ZenEvent>& queue) noexcept;

        /** Unsubscribe an event queue from the sensor */
        void unsubscribe(LockingQueue<ZenEvent>& queue) noexcept;

        /** An data processor associated with this Sensor. It will be destroyed once the sensor
            is destroyed */
        void addProcessor(std::unique_ptr<DataProcessor> processor) noexcept;

        /** Release all processors added to this sensor. This is done outside of the destructor
            to not interfere with the teardown and unsubscribing process when the sensor is
            destroyed */
        void releaseProcessors() noexcept;

    private:
        ZenError processReceivedData(uint8_t address, uint8_t function, gsl::span<const std::byte> data) noexcept override;

        ZenError processReceivedEvent(ZenEvent) noexcept override;

        void publishEvent(const ZenEvent& event) noexcept;

        void upload(std::vector<std::byte> firmware);

        SensorConfig m_config;
        const uintptr_t m_token;
        // [LEGACY]
        std::atomic_bool m_initialized;

        std::mutex m_subscribersMutex;
        std::set<std::reference_wrapper<LockingQueue<ZenEvent>>, ReferenceWrapperCmp<LockingQueue<ZenEvent>>> m_subscribers;

        std::vector<std::unique_ptr<SensorComponent>> m_components;
        std::unique_ptr<ISensorProperties> m_properties;
        std::optional<SyncedModbusCommunicator> m_communicator;
        std::unique_ptr<EventCommunicator> m_eventCommunicator;

        std::atomic_bool m_updatingFirmware;
        std::atomic_bool m_updatedFirmware;
        ZenError m_updateFirmwareError;

        std::atomic_bool m_updatingIAP;
        std::atomic_bool m_updatedIAP;
        ZenError m_updateIAPError;

        std::thread m_uploadThread;

        std::vector<std::unique_ptr<DataProcessor>> m_processors;
    };

    struct SensorCmp
    {
        using is_transparent = std::true_type;

        bool operator()(const std::shared_ptr<Sensor>& lhs, const std::shared_ptr<Sensor>& rhs) const noexcept
        {
            return std::less<uintptr_t>()(lhs->token(), rhs->token());
        }

        bool operator()(const std::shared_ptr<Sensor>& lhs, const ZenSensorHandle_t& rhs) const noexcept
        {
            return std::less<uintptr_t>()(lhs->token(), rhs.handle);
        }

        bool operator()(const ZenSensorHandle& lhs, const std::shared_ptr<Sensor>& rhs) const noexcept
        {
            return std::less<uintptr_t>()(lhs.handle, rhs->token());
        }
    };
}

#endif
