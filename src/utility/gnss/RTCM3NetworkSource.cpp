//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "RTCM3NetworkSource.h"

#include <spdlog/spdlog.h>

#include <asio/placeholders.hpp>

using namespace zen;

RTCM3NetworkSource::RTCM3NetworkSource() : m_transferThread([](TransferThreadOptions& options) {

    const auto localWaitTime = options.m_waitTime;
    auto lmdWaitTimeout = [localWaitTime]() {std::this_thread::sleep_for(std::chrono::seconds(localWaitTime)); };

    if (!options.m_socket->is_open()) {
        asio::ip::address tcpAddress;
        try {
            tcpAddress = asio::ip::address::from_string(options.m_networkHost);
        }
        catch (std::exception & ex) {
            spdlog::error("Can't resolve name {0} due to {1} for RTCM3 corrections", options.m_networkHost, ex.what());
            lmdWaitTimeout();
            return true;
        }

        auto endpoint = asio::ip::tcp::endpoint(tcpAddress, options.m_networkPort);
        try {
            options.m_socket->connect(endpoint);
        }
        catch (const std::exception&) {
            spdlog::error("Connection to name {0} on port {1} failed for RTCM3 corrections",
                options.m_networkHost, options.m_networkPort);
            options.m_socket->close();
            lmdWaitTimeout();
            return true;
        }
    }

    asio::error_code error;
    size_t length = options.m_socket->read_some(asio::buffer(options.m_buffer_read_some), error);
    if (error == asio::error::eof) {
        spdlog::error("Got eof, connection closed by peer for RTCM3 corrections");

        options.m_socket->close();
        lmdWaitTimeout();

        return true;
    }
    else if (error) {
        spdlog::info("Got error while reading RTCM3 corrections, can also occur if the library is shutting down.");
        // try closing and reconnect
        options.m_socket->close();
        lmdWaitTimeout();

        return true;
    }

    for (size_t i = 0; i < length; i++) {
        options.m_parserBuffer.push_back(std::byte(options.m_buffer_read_some[i]));
    }
    while (options.m_parser.next(options.m_parserBuffer) == true) {}

    return true;
    }) {
}

void RTCM3NetworkSource::start(std::string const& networkHost, unsigned long networkPort) {
    m_socket = std::make_unique<asio::ip::tcp::socket>(m_ioContext);

    m_transferThread.start(
        TransferThreadOptions{
            networkHost, uint16_t(networkPort), m_ioContext, m_socket, m_parser
        }
    );

    m_ioContext.run();
}

void RTCM3NetworkSource::stop() {
    m_transferThread.stopAsync();
    if (m_socket->is_open()) {
        asio::error_code ec;
        // this will abort the synchronous read operation in our 
        // readout thread and allow the stream to close (tested on Windows)
        m_socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        m_socket->close();
    }
    m_ioContext.stop();
}

void RTCM3NetworkSource::addFrameCallback(RTCM3Parser::OnFrameCallback const& fc) {
    m_parser.addFrameCallback(fc);
}

