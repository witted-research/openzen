//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef RTCM3_PARSER_H
#define RTCM3_PARSER_H

#include <functional>
#include <cstddef>
#include <vector>
#include <deque>
#include <thread>


/**
Parse RTCM3 packages from a binary stream and output their message number
and the complet binary blob of the packet.
*/

class RTCM3Parser {
public:

    typedef std::function<void(uint16_t, std::vector<std::byte> const&)> OnFrameCallback;

    void addFrameCallback(OnFrameCallback const& fc) {
        m_onFrameCallbacks.push_back(fc);
    }


    /**
    Parses a buffer of arbritary amount of bytes. Returns true if the method
    can be exeucted again to continue parsing on the currrent buffer or false
    if not sufficient data is in the buffer to continue.
    */
    bool next(std::deque<std::byte> & buffer) {
        if (buffer.size() == 0)
            return false;

        if (m_state == ParserState::Preamble) {
            if (buffer.front() == FramePreamble) {
                m_thisFrame.push_back(buffer.front());
                buffer.pop_front();
                m_state = ParserState::Length;
            }
            else {
                // pop this element and look for preamble in the next package
                buffer.pop_front();
            }
        }

        if (m_state == ParserState::Length) {

            if (buffer.size() < 2)
                return false;

            auto byte0 = buffer[0];
            auto byte1 = buffer[1];

            // is big endian
            m_dataLength = (uint16_t(byte0) * 256 + uint16_t(byte1)) & uint16_t(0x3ff);

            buffer.pop_front();
            buffer.pop_front();

            m_thisFrame.push_back(byte0);
            m_thisFrame.push_back(byte1);

            m_state = ParserState::Data;
        }

        if (m_state == ParserState::Data) {

            // crc size at the send is 3 byte
            if (buffer.size() < size_t(m_dataLength + 3))
                return false;

            auto byte0 = buffer[0];
            auto byte1 = buffer[1];

            uint16_t type = (uint16_t(byte0) * 256 + uint16_t(byte1)) >> 4;

            for (uint16_t i = 0; i < m_dataLength + 3; i++) {
                m_thisFrame.push_back(buffer.front());
                buffer.pop_front();
            }

            // done parsing. report callback
            for (auto & fc : m_onFrameCallbacks) {
                fc(type, m_thisFrame);
            }
            m_thisFrame.clear();

            m_state = ParserState::Preamble;
        }

        // in princple, we are ready to continue parsing
        return buffer.size() > 0;
    }

    enum class ParserState {
        Preamble,
        Length,
        Data
    };

private:
    const std::byte FramePreamble = std::byte(0xd3);
    ParserState m_state = ParserState::Preamble;
    uint16_t m_dataLength = 0;
    std::vector<std::byte> m_thisFrame;
    std::vector<OnFrameCallback> m_onFrameCallbacks;
};

#endif