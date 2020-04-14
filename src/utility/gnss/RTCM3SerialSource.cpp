//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "RTCM3SerialSource.h"

#include <spdlog/spdlog.h>
#include <thread>

using namespace zen;

RTCM3SerialSource::RTCM3SerialSource() : m_transferThread([](SerialIoThreadParams& options) {
    // will exit, as soon as m_io.stop is called
    options.m_io.run();
    return false;
    }) {
}

void RTCM3SerialSource::handle_read(const asio::error_code& error,
    std::size_t bytes_transferred) {
    // will be true if there was en error
    if (error)
    {
        // only report as problem if we want to continue reading
        if (m_continueReading) {
            spdlog::error("Encountered error while reading from RTCM3SerialSource: {0}", error.message());

            // wait a moment and try to read again
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        std::string s_in(m_buffer_read_some.begin(), m_buffer_read_some.begin() + bytes_transferred);

        for (size_t i = 0; i < s_in.size(); i++) {
            m_parserBuffer.push_back(std::byte(s_in[i]));
        }
        // parse until parser has nothing left to parse
        while (m_parser.next(m_parserBuffer) == true) {
        }
    }

    // continue reading
    if (m_continueReading) {
        auto lmdReadHandler = [this](const asio::error_code& error, std::size_t bytes_transferred) {
            this->handle_read(error, bytes_transferred);
        };

        m_serial->async_read_some(asio::buffer(m_buffer_read_some, m_buffer_read_some.size()),
            lmdReadHandler);
    }
}

void RTCM3SerialSource::start(std::string const& comPort, unsigned long baudrate) {
    auto lmdTerminateError = [&](std::string const& error) {
        m_continueReading = false;
        if (m_serial) {
            if (m_serial->is_open()) {
                m_serial->cancel();
                m_serial->close();
            }
            m_serial = nullptr;
        }
        if (m_transferThread.isRunning()) {
            m_io.stop();
            m_transferThread.stop();
        }
        spdlog::error(error);
    };

    // reset from previous runs
    m_io.reset();

    m_continueReading = true;
    m_serial = std::make_unique < asio::serial_port>(m_io);
    try {
        m_serial->open(comPort);
    }
    catch (std::exception & e) {
        lmdTerminateError("Cannot open serial port " + comPort + " Error: " + e.what());
        return;
    }

    // serial options tested with mRo SiK Telemetry Radio V2
    m_serial->set_option(asio::serial_port_base::baud_rate(baudrate));
    m_serial->set_option(asio::serial_port::flow_control( asio::serial_port::flow_control::none));    
    m_serial->set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    m_serial->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
    m_serial->set_option(asio::serial_port::character_size(8));

    auto lmdReadHandler = [this](const asio::error_code& error, std::size_t bytes_transferred) {
        this->handle_read(error, bytes_transferred);
        };

    m_serial->async_read_some(asio::buffer(m_buffer_read_some, m_buffer_read_some.size()),
        lmdReadHandler
    );
    
    auto threadParams = SerialIoThreadParams(m_io);
    m_transferThread.start(threadParams);
    spdlog::info("RTCMSerialStream connection opened on serial port {0} with baudrate {1}",
        comPort, baudrate);
}

void RTCM3SerialSource::stop() {
    spdlog::info("RTCMSerialStream transfer stopping");
    m_continueReading = false;
    if (m_serial) {
        m_serial->cancel();
        m_serial->close();
        m_serial = nullptr;
    }
    m_io.stop();
    // actually make sure the worker thread is joined
    m_transferThread.stop();
    spdlog::info("RTCMSerialStream transfer stopped");
}

void RTCM3SerialSource::addFrameCallback(RTCM3Parser::OnFrameCallback const& fc) {
    m_parser.addFrameCallback(fc);
}

