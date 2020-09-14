//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_STREAMING_PROTOCOL_H_
#define ZEN_STREAMING_PROTOCOL_H_

#include "streaming/ZenTypesSerialization.h"
#include "ZenTypes.h"

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>

#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <sstream>

namespace zen {

    namespace Streaming {
        enum StreamingMessageType {
            StreamingMessageType_ZenEventImu = 1,
            StreamingMessageType_ZenEventGnss = 2,
            StreamingMessageType_Unknown = 99
        };

        union StreamingMessagePayload {
            zen::Serialization::ZenEventImuSerialization imuData;
            zen::Serialization::ZenEventGnssSerialization gnssData;
        };

        class StreamingMessage {
        public:
            StreamingMessagePayload payload;
            StreamingMessageType type;
        };

        inline std::optional<ZenEvent> streamingMessageToZenEvent(StreamingMessage const& msg) {
            ZenEvent evt;
            if (msg.type == StreamingMessageType_ZenEventImu) {
                evt.component.handle = msg.payload.imuData.component;
                evt.sensor.handle = msg.payload.imuData.sensor;
                evt.eventType = ZenEventType_ImuData;
                evt.data.imuData = msg.payload.imuData.data;
            }
            else if (msg.type == StreamingMessageType_ZenEventGnss) {
                evt.component.handle = msg.payload.gnssData.component;
                evt.sensor.handle = msg.payload.gnssData.sensor;
                evt.eventType = ZenEventType_GnssData;
                evt.data.gnssData = msg.payload.gnssData.data;
            }
            else {
                spdlog::error("Streaming Message not supported");
                return std::nullopt;
            }

            return evt;
        }

        inline std::optional<StreamingMessage> fromZmqMessage(zmq::message_t & msg) {
            if (msg.size() < 4) {
                return std::nullopt;
            }

            auto received = std::string(static_cast<char*>(msg.data()), msg.size());

            auto header = received.substr(0, 4);
            auto payload = received.substr(4, std::string::npos);

            auto msg_type = StreamingMessageType(header[3]);

            std::stringstream payloadBuffer(payload);

            if (msg_type == StreamingMessageType_ZenEventImu) {
                zen::Serialization::ZenEventImuSerialization imuData;
                {
                    cereal::BinaryInputArchive deser_archive(payloadBuffer);
                    deser_archive(imuData);
                }

                StreamingMessage strMsg;
                strMsg.payload.imuData = imuData;
                strMsg.type = msg_type;
                return strMsg;
            }
            else if (msg_type == StreamingMessageType_ZenEventGnss) {
                zen::Serialization::ZenEventGnssSerialization gnssData;
                {
                    cereal::BinaryInputArchive deser_archive(payloadBuffer);
                    deser_archive(gnssData);
                }

                StreamingMessage strMsg;
                strMsg.payload.gnssData = gnssData;
                strMsg.type = msg_type;
                return strMsg;
            } else {
            spdlog::error("Zmq Streaming message of type {0} not supported", msg_type);
            }

            return std::nullopt;
        }

        template <class TPayload>
        inline void copyToZmqMessage(zen::Streaming::StreamingMessageType msgType,
            TPayload const& payload, zmq::message_t & zmqOut) {

            // header has 4 bytes
            std::vector<std::byte> completeBuffer;
            completeBuffer.push_back(std::byte(0));
            completeBuffer.push_back(std::byte(0));
            completeBuffer.push_back(std::byte(0));
            completeBuffer.push_back(std::byte(msgType));

            std::stringstream buffer;
            {
                cereal::BinaryOutputArchive ser_archive(buffer);
                ser_archive(payload);
            }

            auto sBuffer = buffer.str();
            for (auto s : sBuffer) {
                completeBuffer.push_back(std::byte(s));
            }

            zmqOut.rebuild(completeBuffer.data(), completeBuffer.size());
        }

        inline bool toZmqMessage(ZenEvent const& evt, zmq::message_t & zmqOut) {
            // todo: this needs to be refactored when the event type numbering scheme is fixed
            // right now the component numbers for IMU and GNSS are hard-coded
            if (evt.component.handle == 1) {

                zen::Serialization::ZenEventImuSerialization imuData;
                imuData.sensor = evt.sensor.handle;
                imuData.component = evt.component.handle;
                imuData.data = evt.data.imuData;

                copyToZmqMessage(zen::Streaming::StreamingMessageType_ZenEventImu,
                    imuData, zmqOut);
                return true;
            } else if (evt.component.handle == 2) {
                zen::Serialization::ZenEventGnssSerialization gnssData;
                gnssData.sensor = evt.sensor.handle;
                gnssData.component = evt.component.handle;
                gnssData.data = evt.data.gnssData;

                copyToZmqMessage(zen::Streaming::StreamingMessageType_ZenEventGnss,
                    gnssData, zmqOut);
                return true;
            }

            // message cannot be streamed
            return false;
        }
    }
}

#endif
