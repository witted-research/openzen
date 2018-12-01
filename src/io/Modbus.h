#ifndef ZEN_IO_MODBUS_H_
#define ZEN_IO_MODBUS_H_

#include <cstdint>
#include <vector>

namespace zen::modbus
{
    struct Frame
    {
        std::vector<unsigned char> data;
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

    class IFrameParser
    {
    protected:
        ~IFrameParser() = default; // Prevent usage of base class

    public:
        virtual FrameParseError parse(const unsigned char* data, size_t& length) = 0;
        virtual void reset();

        virtual bool finished() const = 0;

        Frame&& frame() { return std::move(m_frame); }
        const Frame& frame() const { return m_frame; }

    protected:
        Frame m_frame;
    };

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

        FrameParseError parse(const unsigned char* data, size_t& length) override;
        void reset() override;

        bool finished() const override { return m_state == ASCIIFrameParseState::Finished; }

    private:
        ASCIIFrameParseState m_state;
        uint8_t m_length;
        unsigned char m_buffer;
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

    class RTUFrameParser : public IFrameParser
    {
    public:
        RTUFrameParser();

        FrameParseError parse(const unsigned char* data, size_t& length) override;
        void reset() override;

        bool finished() const override { return m_state == RTUFrameParseState::Finished; }

    private:
        RTUFrameParseState m_state;
        uint8_t m_length;
        unsigned char m_buffer;
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

    class LpFrameParser : public IFrameParser
    {
    public:
        LpFrameParser();

        FrameParseError parse(const unsigned char* data, size_t& length) override;
        void reset() override;

        bool finished() const override { return m_state == LpFrameParseState::Finished; }

    private:
        LpFrameParseState m_state;
        uint8_t m_length;
        unsigned char m_buffer;
    };

    std::vector<unsigned char> makeASCIIFrame(uint8_t address, uint8_t function, const unsigned char* data, uint8_t length);
    std::vector<unsigned char> makeLpFrame(uint8_t address, uint8_t function, const unsigned char* data, uint8_t length);
    std::vector<unsigned char> makeRTUFrame(uint8_t address, uint8_t function, const unsigned char* data, uint8_t length);
}

#endif