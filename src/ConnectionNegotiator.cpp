#include "ConnectionNegotiator.h"

#include <chrono>

#include "nlohmann/json.hpp"

#include "ZenProtocol.h"
#include "utility/Finally.h"

namespace zen
{
    ConnectionNegotiator::ConnectionNegotiator(BaseIoInterface& ioInterface) noexcept
        : IIoDataSubscriber(ioInterface)
        , m_ioInterface(ioInterface)
        , m_error(ZenSensorInitError_None)
        , m_terminated(false)
    {}

    nonstd::expected<SensorConfig, ZenSensorInitError> ConnectionNegotiator::negotiate() noexcept
    {
        constexpr const auto IO_TIMEOUT = std::chrono::milliseconds(2000);
        std::vector<unsigned int> baudrates;

        for (uint32_t baudrate : baudrates)
        {
            if (auto error = m_ioInterface.setBaudrate(baudrate))
                return nonstd::make_unexpected(ZenSensorInitError_IncompatibleBaudRates);

            if (auto error = m_ioInterface.send(0, ZenProtocolFunction_Negotiate, reinterpret_cast<const unsigned char*>(&baudrate), sizeof(baudrate)))
                return nonstd::make_unexpected(ZenSensorInitError_SendFailed);

            std::unique_lock<std::mutex> lock;
            if (m_cv.wait_for(lock, IO_TIMEOUT, [this]() { return m_terminated; }))
            {
                if (m_error)
                    return nonstd::make_unexpected(m_error);

                return std::move(m_config);
            }
        }

        // [LEGACY] Fix for sensors that did not support negotiation yet
        //return nonstd::make_unexpected(ZenSensorInitError_IncompatibleBaudRates);
        m_ioInterface.setBaudrate(921600);
        m_config.version = 0u;

        ComponentConfig imu;
        imu.id = g_zenSensorType_Imu;
        imu.version = 0;
        m_config.components.push_back(std::move(imu));

        return std::move(m_config);

    }

    ZenError ConnectionNegotiator::processData(uint8_t, uint8_t function, const unsigned char* data, size_t length)
    {
        if (function != ZenProtocolFunction_Handshake)
            return ZenError_Io_UnexpectedFunction;

        using namespace nlohmann;

        auto guard = finally([this]() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_terminated = true;
            lock.unlock();

            m_cv.notify_one();
        });

        json config = json::parse(data, data + length, nullptr, false);
        const auto versionIt = config.find("version");
        const auto componentsIt = config.find("components");
        if (versionIt == config.end() || !versionIt->is_number_integer() ||
            componentsIt == config.end() || !componentsIt->is_array())
        {
            m_error = ZenSensorInitError_InvalidConfig;
            return ZenError_InvalidArgument;
        }
         
        m_config.version = *versionIt;
        auto components = *componentsIt;
        for (size_t idx = 0; idx < components.size(); ++idx)
        {
            auto component = components[idx];
            const auto nameIt = component.find("name");
            const auto compVersionIt = component.find("version");
            if (nameIt == component.end() || !nameIt->is_string() ||
                compVersionIt == component.end() || !compVersionIt->is_number_integer())
            {
                m_error = ZenSensorInitError_InvalidConfig;
                return ZenError_InvalidArgument;
            }

            ComponentConfig compConfig;
            compConfig.id = *nameIt->get_ptr<json::string_t*>();
            compConfig.version = static_cast<uint32_t>(*components[idx].get_ptr<json::number_unsigned_t*>());

            m_config.components.push_back(std::move(compConfig));
        }

        return ZenError_None;
    }
}