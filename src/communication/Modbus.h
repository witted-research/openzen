//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMMUNICATION_MODBUS_H_
#define ZEN_COMMUNICATION_MODBUS_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <gsl/span>

namespace zen::modbus
{
    struct Frame
    {
        std::vector<std::byte> data;
        uint8_t address;
        uint8_t function;
    };

    typedef enum FrameParseError : uint8_t
    {
        FrameParseError_None,

        FrameParseError_ExpectedStart,
        FrameParseError_ChecksumInvalid,
        FrameParseError_UnexpectedCharacter,
        FrameParseError_ExpectedEnd,
        FrameParseError_Finished,

        FrameParseError_Max
    } FrameParseError;

    class IFrameFactory
    {
    public:
        virtual ~IFrameFactory() = default;

        virtual std::vector<std::byte> makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const = 0;
    };

    class IFrameParser
    {
    public:
        virtual ~IFrameParser() = default;

    public:
        virtual FrameParseError parse(gsl::span<const std::byte>& data) = 0;
        virtual void reset();

        virtual bool finished() const = 0;

        Frame&& frame() { return std::move(m_frame); }
        const Frame& frame() const { return m_frame; }

    protected:
        Frame m_frame;
    };

    enum class ModbusFormat
    {
        ASCII,
        LP,
        RTU
    };

    std::unique_ptr<IFrameFactory> make_factory(ModbusFormat format) noexcept;
    std::unique_ptr<IFrameParser> make_parser(ModbusFormat format) noexcept;

    enum class ASCIIFrameParseState
    {
        Start,
        Address1,
        Address2,
        Function1,
        Function2,
        Length1,
        Length2,
        Data1,
        Data2,
        Check1,
        Check2,
        End1,
        End2,
        Finished,

        Max
    };

    class ASCIIFrameParser : public IFrameParser
    {
    public:
        ASCIIFrameParser();

        FrameParseError parse(gsl::span<const std::byte>& data) override;
        void reset() override;

        bool finished() const override { return m_state == ASCIIFrameParseState::Finished; }

    private:
        ASCIIFrameParseState m_state;
        uint8_t m_length;
        std::byte m_buffer;
    };

    class ASCIIFrameFactory : public IFrameFactory
    {
        std::vector<std::byte> makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const override;
    };

    enum class RTUFrameParseState
    {
        Address,
        Function,
        Length,
        Data,
        Check1,
        Check2,
        Finished,

        Max
    };

    class RTUFrameFactory : public IFrameFactory
    {
        std::vector<std::byte> makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const override;
    };

    class RTUFrameParser : public IFrameParser
    {
    public:
        RTUFrameParser();

        FrameParseError parse(gsl::span<const std::byte>& data) override;
        void reset() override;

        bool finished() const override { return m_state == RTUFrameParseState::Finished; }

    private:
        RTUFrameParseState m_state;
        uint8_t m_length;
        std::byte m_buffer;
    };

    enum class LpFrameParseState
    {
        Start,
        Address1,
        Address2,
        Function1,
        Function2,
        Length1,
        Length2,
        Data,
        Check1,
        Check2,
        End1,
        End2,
        Finished,

        Max
    };

    class LpFrameFactory : public IFrameFactory
    {
        std::vector<std::byte> makeFrame(uint8_t address, uint8_t function, const std::byte* data, uint8_t length) const override;
    };

    class LpFrameParser : public IFrameParser
    {
    public:
        LpFrameParser();

        FrameParseError parse(gsl::span<const std::byte>& data) override;
        void reset() override;

        bool finished() const override { return m_state == LpFrameParseState::Finished; }

    private:
        LpFrameParseState m_state;
        uint8_t m_length;
        std::byte m_buffer;
    };
}

#endif
