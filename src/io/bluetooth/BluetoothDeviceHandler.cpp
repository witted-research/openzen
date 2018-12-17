#include "io/bluetooth/BluetoothDeviceHandler.h"

#include "qbluetoothservicediscoveryagent.h"

namespace zen
{
    BluetoothDeviceHandler::BluetoothDeviceHandler(uint64_t address)
        : m_address(address)
        , m_disconnected(false)
        , m_error(ZenError_None)
    {}

    BluetoothDeviceHandler::~BluetoothDeviceHandler()
    {
        quit();
        wait();
    }

    ZenError BluetoothDeviceHandler::initialize()
    {
        start();
        m_fence.wait();

        ZenError result = m_error;
        reset();
        return result;
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

    std::optional<std::vector<unsigned char>> BluetoothDeviceHandler::tryToGetReceivedData()
    {
        return m_queue.tryToPop();
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
                m_fence.terminate();
            }, Qt::DirectConnection);

            connect(m_socket.get(), &QBluetoothSocket::disconnected, this, [this]() {
                m_disconnected = true;
            }, Qt::DirectConnection);

            connect(m_socket.get(), QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this, [this]() {
                m_error = ZenError_Io_InitFailed;
                m_fence.terminate();
            }, Qt::DirectConnection);

            connect(m_socket.get(), &QBluetoothSocket::readyRead, this, &BluetoothDeviceHandler::receiveData, Qt::DirectConnection);

            m_socket->connectToService(service.device().address(), service.serviceUuid());
        }
    }

    void BluetoothDeviceHandler::receiveData()
    {
        QByteArray data = m_socket->readAll();
        std::vector<unsigned char> buffer(data.begin(), data.end());
        m_queue.emplace(std::move(buffer));
    }

    void BluetoothDeviceHandler::reset()
    {
        m_error = ZenError_None;
        m_fence.reset();
    }
}
