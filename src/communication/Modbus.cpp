//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#include "Modbus.h"

#include <stdexcept>

namespace
{
    constexpr const uint16_t crc16IbmLut[256] = {
        0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
        0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
        0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
        0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
        0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
        0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
        0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
        0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
        0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
        0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
        0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
        0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
        0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
        0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
        0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
        0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
        0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
        0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
        0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
        0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
        0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
        0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
        0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
        0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
        0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
        0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
        0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
        0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
        0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
        0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
        0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
        0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
    };

    constexpr uint16_t crc16Byte(uint16_t crc, uint8_t b)
    {
        const auto idx = (crc ^ uint16_t(b)) & 0xff;
        return (crc >> 8) ^ crc16IbmLut[idx];
    }

    constexpr uint16_t crc16(uint16_t crc, uint8_t address, uint8_t function, const std::byte* data, uint8_t length)
    {
        crc = crc16Byte(crc, address);
        crc = crc16Byte(crc, function);
        crc = crc16Byte(crc, length);

        for (auto i = 0; i < length; ++i)
            crc = crc16Byte(crc, std::to_integer<uint8_t>(data[i]));

        return crc;
    }

    uint8_t lrc(uint8_t address, uint8_t function, const std::byte* data, uint8_t length)
    {
        uint8_t total = address;
        total += function;
        total += length;

        for (auto i = 0; i < length; ++i)
            total += std::to_integer<uint8_t>(data[i]);

        return total ^ 0b11111111;
    }

    uint16_t lrcLp(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) noexcept
    {
        // TODO:
        // LP Sensor firmware computes the additions for
        // the checksum on a byte-level and not using address
        // and function 2-byte integers, check this is working
        // here
        uint16_t total = address;
        total += function;
        total += length;

        for (auto i = 0; i < length; ++i)
            total += std::to_integer<uint8_t>(data[i]);

        return total;
    }

    uint8_t leastSigHex(uint8_t byte) noexcept
    {
        return byte & 0b00001111;
    }

    uint8_t mostSigHex(uint8_t byte) noexcept
    {
        return (byte >> 4) & 0b00001111;
    }

    uint16_t combine(uint8_t least, uint8_t most) noexcept
    {
        return (uint16_t(most) << 8) | least;
    }

    std::byte hexToByte(uint8_t least, uint8_t most) noexcept
    {
        return std::byte((most << 4) | least);
    }

    std::byte toASCII(uint8_t i) noexcept
    {
        constexpr unsigned char dectoASCII[] { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        return std::byte(dectoASCII[i]);
    }

    uint8_t fromASCII(std::byte b) noexcept
    {
        const unsigned char c = std::to_integer<unsigned char>(b);
        if (c >= 'A')
            return c - 55; // A-F (10-15)

        return c - 48; // 0-9
    }

    bool isHex(std::byte b) noexcept
    {
        const unsigned char c = std::to_integer<unsigned char>(b);
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    }
}

namespace zen::modbus
{
    void IFrameParser::reset()
    {
        m_frame.data.clear();
    }

    std::unique_ptr<IFrameFactory> make_factory(ModbusFormat format) noexcept
    {
        switch (format)
        {
        case ModbusFormat::ASCII:
            return std::make_unique<ASCIIFrameFactory>();

        case ModbusFormat::LP:
            return std::make_unique<LpFrameFactory>();

        case ModbusFormat::RTU:
            return std::make_unique<RTUFrameFactory>();

        default:
            return {};
        }
    }

    std::unique_ptr<IFrameParser> make_parser(ModbusFormat format) noexcept
    {
        switch (format)
        {
        case ModbusFormat::ASCII:
            return std::make_unique<ASCIIFrameParser>();

        case ModbusFormat::LP:
            return std::make_unique<LpFrameParser>();

        case ModbusFormat::RTU:
            return std::make_unique<RTUFrameParser>();

        default:
            return {};
        }
    }

    std::vector<std::byte> ASCIIFrameFactory::makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const
    {
        constexpr uint8_t WRAPPER_SIZE = 9; // 1 (start) + 2 (address) + 2 (function) + 2 (LRC) + 2 (end)
        std::vector<std::byte> frame(WRAPPER_SIZE + 2 + 2 * length);

        frame[0] = std::byte(0x3a); // Start with colon :
        frame[1] = toASCII(leastSigHex(address));
        frame[2] = toASCII(mostSigHex(address));
        frame[3] = toASCII(leastSigHex(function));
        frame[4] = toASCII(mostSigHex(function));
        frame[5] = toASCII(leastSigHex(length));
        frame[6] = toASCII(mostSigHex(length));

        for (auto i = 0; i < length; ++i)
        {
            frame[7 + 2 * i] = toASCII(leastSigHex(std::to_integer<uint8_t>(data[i])));
            frame[7 + 2 * i + 1] = toASCII(mostSigHex(std::to_integer<uint8_t>(data[i])));
        }

        const uint8_t checksum = lrc(address, function, data, length);
        frame[7 + 2 * length] = toASCII(leastSigHex(checksum));
        frame[8 + 2 * length] = toASCII(mostSigHex(checksum));
        frame[9 + 2 * length] = std::byte(0x0d); // Carriage Return
        frame[10 + 2 * length] = std::byte(0x0a); // Line Feed

        return frame;
    }

    ASCIIFrameParser::ASCIIFrameParser()
        : m_state(ASCIIFrameParseState::Start)
    {}

    void ASCIIFrameParser::reset()
    {
        IFrameParser::reset();
        m_state = ASCIIFrameParseState::Start;
    }

    FrameParseError ASCIIFrameParser::parse(gsl::span<const std::byte>& data)
    {
        while (!data.empty())
        {
            // Separate error checking of hexadecimal values for cleanliness
            if (m_state > ASCIIFrameParseState::Start && m_state < ASCIIFrameParseState::End1)
                if (!isHex(data[0]))
                    return FrameParseError_UnexpectedCharacter;

            switch (m_state)
            {
            case ASCIIFrameParseState::Start:
                if (data[0] != std::byte(0x3a))
                    return FrameParseError_ExpectedStart;

                m_state = ASCIIFrameParseState::Address1;
                break;

            case ASCIIFrameParseState::Address1:
                m_buffer = data[0];
                m_state = ASCIIFrameParseState::Address2;
                break;

            case ASCIIFrameParseState::Address2:
                m_frame.address = std::to_integer<uint8_t>(hexToByte(fromASCII(m_buffer), fromASCII(data[0])));
                m_state = ASCIIFrameParseState::Function1;
                break;

            case ASCIIFrameParseState::Function1:
                m_buffer = data[0];
                m_state = ASCIIFrameParseState::Function2;
                break;

            case ASCIIFrameParseState::Function2:
                m_frame.function = std::to_integer<uint8_t>(hexToByte(fromASCII(m_buffer), fromASCII(data[0])));
                m_state = ASCIIFrameParseState::Length1;
                break;

            case ASCIIFrameParseState::Length1:
                m_buffer = data[0];
                m_state = ASCIIFrameParseState::Length2;
                break;

            case ASCIIFrameParseState::Length2:
                m_length = std::to_integer<uint8_t>(hexToByte(fromASCII(m_buffer), fromASCII(data[0])));
                m_frame.data.reserve(m_length);
                m_state = m_length != 0 ? ASCIIFrameParseState::Data1 : ASCIIFrameParseState::Check1;
                break;

            case ASCIIFrameParseState::Data1:
                m_buffer = data[0];
                m_state = ASCIIFrameParseState::Data2;
                break;

            case ASCIIFrameParseState::Data2:
                m_frame.data.emplace_back(hexToByte(fromASCII(m_buffer), fromASCII(data[0])));
                m_state = m_frame.data.size() == m_length ? ASCIIFrameParseState::Check1 : ASCIIFrameParseState::Data1;
                break;

            case ASCIIFrameParseState::Check1:
                m_buffer = data[0];
                m_state = ASCIIFrameParseState::Check2;
                break;

            case ASCIIFrameParseState::Check2:
                if (hexToByte(fromASCII(m_buffer), fromASCII(data[0])) != std::byte(lrc(m_frame.address, m_frame.function, m_frame.data.data(), m_length)))
                    return FrameParseError_ChecksumInvalid;

                m_state = ASCIIFrameParseState::End1;
                break;
                
            case ASCIIFrameParseState::End1:
                if (data[0] != std::byte(0x0d))
                    return FrameParseError_ExpectedEnd;

                m_state = ASCIIFrameParseState::End2;
                break;

            case ASCIIFrameParseState::End2:
                if (data[0] != std::byte(0x0a))
                    return FrameParseError_ExpectedEnd;

                m_state = ASCIIFrameParseState::Finished;
                data = data.subspan(1);
                return FrameParseError_None;

            case ASCIIFrameParseState::Finished:
                return FrameParseError_Finished;

            default:
                return FrameParseError_UnexpectedCharacter;
            }

            data = data.subspan(1);
        }

        return FrameParseError_None;
    }

    std::vector<std::byte> LpFrameFactory::makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const
    {
        constexpr uint8_t WRAPPER_SIZE = 9; // 1 (start) + 2 (address) + 2 (function) + 2 (LRC) + 2 (end)
        std::vector<std::byte> frame(WRAPPER_SIZE + 2 + length);

        frame[0] = std::byte(0x3a);
        frame[1] = std::byte(address);
        frame[2] = std::byte(0);
        frame[3] = std::byte(function);
        frame[4] = std::byte(0);
        frame[5] = std::byte(length);
        frame[6] = std::byte(0);
        if (length > 0) {
            std::copy(data, data + length, &frame[7]);
        }

        const uint16_t checksum = lrcLp(address, function, data, length);
        frame[7 + length] = std::byte(checksum & 0xff);
        frame[8 + length] = std::byte((checksum >> 8) & 0xff);
        frame[9 + length] = std::byte(0x0d);
        frame[10 + length] = std::byte(0x0a);

        return frame;
    }

    LpFrameParser::LpFrameParser()
        : m_state(LpFrameParseState::Start)
    {}

    void LpFrameParser::reset()
    {
        IFrameParser::reset();
        m_state = LpFrameParseState::Start;
    }

    FrameParseError LpFrameParser::parse(gsl::span<const std::byte>& data)
    {
        while (!data.empty())
        {
            switch (m_state)
            {
            case LpFrameParseState::Start:
                if (data[0] != std::byte(0x3a))
                    return FrameParseError_ExpectedStart;

                m_state = LpFrameParseState::Address1;
                break;

            case LpFrameParseState::Address1:
                m_buffer = data[0];
                m_state = LpFrameParseState::Address2;
                break;

            case LpFrameParseState::Address2:
                m_frame.address = std::to_integer<uint8_t>(m_buffer);
                m_state = LpFrameParseState::Function1;
                break;

            case LpFrameParseState::Function1:
                m_buffer = data[0];
                m_state = LpFrameParseState::Function2;
                break;

            case LpFrameParseState::Function2:
                m_frame.function = std::to_integer<uint8_t>(m_buffer);
                m_state = LpFrameParseState::Length1;
                break;

            case LpFrameParseState::Length1:
                m_buffer = data[0];
                m_state = LpFrameParseState::Length2;
                break;

            case LpFrameParseState::Length2:
                m_length = static_cast<uint8_t>(combine(std::to_integer<uint8_t>(m_buffer), std::to_integer<uint8_t>(data[0])));
                m_frame.data.reserve(m_length);
                m_state = m_length != 0 ? LpFrameParseState::Data : LpFrameParseState::Check1;
                break;

            case LpFrameParseState::Data:
                m_frame.data.emplace_back(data[0]);
                m_state = m_frame.data.size() == m_length ? LpFrameParseState::Check1 : LpFrameParseState::Data;
                break;

            case LpFrameParseState::Check1:
                m_buffer = data[0];
                m_state = LpFrameParseState::Check2;
                break;

            case LpFrameParseState::Check2:
                if (combine(std::to_integer<uint8_t>(m_buffer), std::to_integer<uint8_t>(data[0])) != lrcLp(m_frame.address, m_frame.function, m_frame.data.data(), m_length))
                    return FrameParseError_ChecksumInvalid;

                m_state = LpFrameParseState::End1;
                break;

            case LpFrameParseState::End1:
                if (data[0] != std::byte(0x0d))
                    return FrameParseError_ExpectedEnd;

                m_state = LpFrameParseState::End2;
                break;

            case LpFrameParseState::End2:
                if (data[0] != std::byte(0x0a))
                    return FrameParseError_ExpectedEnd;

                m_state = LpFrameParseState::Finished;
                data = data.subspan(1);
                return FrameParseError_None;

            case LpFrameParseState::Finished:
                return FrameParseError_Finished;

            default:
                return FrameParseError_UnexpectedCharacter;
            }

            data = data.subspan(1);
        }

        return FrameParseError_None;
    }

    // Requires a wait of 3.5 character times
    std::vector<std::byte> RTUFrameFactory::makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const
    {
        constexpr size_t WRAPPER_SIZE = 4; // 1 (address) + 1 (function) + 2 (CRC)
        std::vector<std::byte> frame(WRAPPER_SIZE + 1 + length);

        frame[0] = std::byte(address);
        frame[1] = std::byte(function);
        frame[2] = std::byte(length);
        if (length > 0) {
            std::copy(data, data + length, &frame[3]);
        }

        const uint16_t checksum = crc16(0xffff, address, function, data, length);
        frame[3 + length] = std::byte(checksum & 0xff);
        frame[4 + length] = std::byte((checksum >> 8) & 0xff);

        return frame;
    }

    RTUFrameParser::RTUFrameParser()
        : m_state(RTUFrameParseState::Address)
    {}

    void RTUFrameParser::reset()
    {
        IFrameParser::reset();
        m_state = RTUFrameParseState::Address;
    }

    FrameParseError RTUFrameParser::parse(gsl::span<const std::byte>& data)
    {
        while (!data.empty())
        {
            switch (m_state)
            {
            case RTUFrameParseState::Address:
                m_frame.address = std::to_integer<uint8_t>(data[0]);
                m_state = RTUFrameParseState::Function;
                break;

            case RTUFrameParseState::Function:
                m_frame.function = std::to_integer<uint8_t>(data[0]);
                m_state = RTUFrameParseState::Length;
                break;

            case RTUFrameParseState::Length:
                m_length = std::to_integer<uint8_t>(data[0]);
                m_frame.data.reserve(m_length);
                m_state = m_length != 0 ? RTUFrameParseState::Data : RTUFrameParseState::Check1;
                break;

            case RTUFrameParseState::Data:
                m_frame.data.emplace_back(data[0]);
                m_state = m_frame.data.size() == m_length ? RTUFrameParseState::Check1 : RTUFrameParseState::Data;
                break;

            case RTUFrameParseState::Check1:
                m_buffer = data[0];
                m_state = RTUFrameParseState::Check2;
                break;

            case RTUFrameParseState::Check2:
                if (combine(std::to_integer<uint8_t>(m_buffer), std::to_integer<uint8_t>(data[0])) != crc16(0xffff, m_frame.address, m_frame.function, m_frame.data.data(), m_length))
                    return FrameParseError_ChecksumInvalid;

                m_state = RTUFrameParseState::Finished;
                data = data.subspan(1);
                return FrameParseError_None;

            case RTUFrameParseState::Finished:
                return FrameParseError_Finished;

            default:
                return FrameParseError_UnexpectedCharacter;
            }

            data = data.subspan(1);
        }

        return FrameParseError_None;
    }
}
