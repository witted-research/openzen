#include "io/bluetooth/BluetoothDeviceHandler.h"

#include "qbluetoothservicediscoveryagent.h"

namespace zen
{
    BluetoothDeviceHandler::BluetoothDeviceHandler(uint64_t address)
        : m_address(address)
        , m_connected(false)
        , m_error(ZenError_None)
    {}

    BluetoothDeviceHandler::~BluetoothDeviceHandler()
    {
        quit();
        wait();

        auto promise = m_promise.borrow();
        if (promise->has_value())
            promise->value().set_value(nonstd::make_unexpected(ZenError_None));
    }

    ZenError BluetoothDeviceHandler::initialize()
    {
        start();
        if (m_fence.waitFor(std::chrono::milliseconds(5000)))
        {
            ZenError result = m_error;
            reset();
            return result;
        }
        return ZenError_Io_InitFailed;
    }

    void BluetoothDeviceHandler::run()
    {
        QBluetoothServiceDiscoveryAgent agent;
        agent.setRemoteAddress(m_address);

        connect(&agent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothDeviceHandler::serviceDiscovered, Qt::DirectConnection);
        connect(&agent, QOverload<QBluetoothServiceDiscoveryAgent::Error>::of(&QBluetoothServiceDiscoveryAgent::error), this, [this]() {
            m_error = ZenError_Io_InitFailed;
            quit();
        }, Qt::DirectConnection);
        connect(&agent, &QBluetoothServiceDiscoveryAgent::finished, this, [this]() {
            if (!m_socket)
            {
                m_error = ZenError_Io_InitFailed;
                quit();
            }
        }, Qt::DirectConnection);

        agent.start();
        exec();
    }

    nonstd::expected<std::future<BluetoothDeviceHandler::async_read_value_t>, ZenAsyncStatus> BluetoothDeviceHandler::readAsync()
    {
        auto optPromise = m_promise.borrow();
        if (optPromise->has_value())
            return nonstd::make_unexpected(ZenAsync_ThreadBusy);

        std::promise<async_read_value_t> promise;
        auto future = promise.get_future();

        auto buffer = m_buffer.borrow();
        if (!buffer->empty())
            promise.set_value(std::move(*buffer));
        else
            optPromise->emplace(std::move(promise));

        return future;
    }

    ZenError BluetoothDeviceHandler::send(const std::vector<unsigned char>& data)
    {
        auto it = reinterpret_cast<const char*>(data.data());
        auto end = reinterpret_cast<const char*>(data.data() + data.size());
        while (it != end)
        {
            auto nBytesWritten = m_socket->write(it, std::distance(it, end));
            if (nBytesWritten == -1)
                return ZenError_Io_SendFailed;

            it += nBytesWritten;
        }

        return ZenError_None;
    }

    bool BluetoothDeviceHandler::equals(uint64_t handle) const
    {
        return m_address.toUInt64() == handle;
    }

    void BluetoothDeviceHandler::serviceDiscovered(const QBluetoothServiceInfo& service)
    {
        if (!m_socket)
        {
            m_socket = std::make_unique<QBluetoothSocket>(QBluetoothServiceInfo::RfcommProtocol);

            connect(m_socket.get(), &QBluetoothSocket::connected, this, [this]() {
                m_connected = true;
                m_fence.terminate();
            }, Qt::DirectConnection);

            connect(m_socket.get(), &QBluetoothSocket::disconnected, this, [this]() {
                m_connected = false;
            }, Qt::DirectConnection);

            connect(m_socket.get(), QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, [this](QBluetoothSocket::SocketError error) {
                if (m_connected)
                {
                    m_connected = false;
                    if (error == QBluetoothSocket::SocketError::RemoteHostClosedError)
                    {
                        auto optPromise = m_promise.borrow();
                        if (optPromise->has_value())
                            optPromise->value().set_value(nonstd::make_unexpected(ZenError_Io_NotInitialized));
                    }
                }
                else
                {
                    m_error = ZenError_Io_InitFailed;
                    m_fence.terminate();
                }
            }, Qt::DirectConnection);

            connect(m_socket.get(), &QBluetoothSocket::readyRead, this, &BluetoothDeviceHandler::receiveData, Qt::DirectConnection);
            m_socket->connectToService(service.device().address(), service.serviceUuid());
        }
    }

    void BluetoothDeviceHandler::receiveData()
    {
        QByteArray data = m_socket->readAll();

        auto promise = m_promise.borrow();
        auto buffer = m_buffer.borrow();
        buffer->insert(buffer->end(), data.begin(), data.end());

        if (promise->has_value())
        {
            promise->value().set_value(std::move(*buffer));
            promise->reset();
        }
    }

    void BluetoothDeviceHandler::reset()
    {
        m_error = ZenError_None;
        m_fence.reset();
    }
}
