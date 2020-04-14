//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef RTCM3_NETWORK_SOURCE_H
#define RTCM3_NETWORK_SOURCE_H

#include "RTCM3Parser.h"

#include "utility/ManagedThread.h"

#include <cstddef>
#include <vector>
#include <deque>
#include <thread>

#include <asio/io_service.hpp>
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <asio/ip/tcp.hpp>

namespace zen {

    class RTCM3NetworkSource {
    public:

        RTCM3NetworkSource();

        void start(std::string const& networkHost, unsigned long networkPort);

        void stop();

        /**
        Can only be called before or after start call
        */
        void addFrameCallback(RTCM3Parser::OnFrameCallback const& fc);

    private:

        asio::io_context m_ioContext;
        std::unique_ptr<asio::ip::tcp::socket> m_socket;
        RTCM3Parser m_parser;

        class TransferThreadOptions {
        public:
            const std::string m_networkHost;
            const uint16_t m_networkPort;
            asio::io_context& m_ioContext;
            std::unique_ptr<asio::ip::tcp::socket>& m_socket;
            RTCM3Parser& m_parser;

            // will be created by the thread
            std::array<char, 1024> m_buffer_read_some{};
            std::deque<std::byte> m_parserBuffer{};
            std::chrono::seconds m_waitTime = std::chrono::seconds(1);
        };

        ManagedThread< TransferThreadOptions> m_transferThread;
    };

}

#endif // !RTCM3_NETWORK_SOURCE_H
