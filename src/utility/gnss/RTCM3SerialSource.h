//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef RTCM3_SERIAL_SOURCE_H
#define RTCM3_SERIAL_SOURCE_H

#include "RTCM3Parser.h"

#include "utility/ManagedThread.h"

#include <cstddef>
#include <deque>
#include <thread>

#include <asio/io_service.hpp>
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <asio/serial_port.hpp>

namespace zen {

    /**
    Reading RTCM3 correction data from a serial port (for example for a
    mRo SiK Telemetry Radio V2 connected via USB).

    Note: there is a bug in recent (newer than 2019/07) asio implementations
    which prevent opening of COM-Ports on windows
    https://github.com/boostorg/asio/pull/273/files
    Make sure this bug is fixed in asio standalone when updating the asio external
    next time.
    */
    class RTCM3SerialSource {
    public:

        RTCM3SerialSource();

        void start(std::string const& networkHost, unsigned long networkPort);

        void stop();

        /**
        Can only be called before or after start call
        */
        void addFrameCallback(RTCM3Parser::OnFrameCallback const& fc);

    private:
        void handle_read(const asio::error_code& error,
            std::size_t bytes_transferred);

        RTCM3Parser m_parser;
        std::deque<std::byte> m_parserBuffer;

        std::array<char, 1024> m_buffer_read_some;
        std::atomic_bool m_continueReading = true;
        asio::io_service m_io;

        class SerialIoThreadParams {
        public:
            SerialIoThreadParams(asio::io_service & io) : m_io(io) {
            }

            asio::io_service & m_io;
        };

        ManagedThread<SerialIoThreadParams> m_transferThread;
        std::unique_ptr<asio::serial_port> m_serial;
    };
}

#endif // !RTCM3_SERIAL_SOURCE_H
