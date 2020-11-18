//===========================================================================//
//
// Copyright (C) 2020 LP-Research Inc.
//
// This file is part of OpenZen, under the MIT License.
// See https://bitbucket.org/lpresearch/openzen/src/master/LICENSE for details
// SPDX-License-Identifier: MIT
//
//===========================================================================//

#ifndef ZEN_COMMUNICATION_MOCKMODBUSCOMMUNICATOR_H_
#define ZEN_COMMUNICATION_MOCKMODBUSCOMMUNICATOR_H_

#include "communication/Modbus.h"
#include "communication/ModbusCommunicator.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <thread>
#include <future>
#include <chrono>

namespace zen {

class DummyFrameFactory : public modbus::IFrameFactory {
public:
 std::vector<std::byte> makeFrame(uint8_t, uint8_t,
   const std::byte*, uint8_t) const override{
    return {};
  }
};

class DummyFrameParser : public modbus::IFrameParser {
public:
  modbus::FrameParseError parse(gsl::span<const std::byte>&) override {
    return modbus::FrameParseError_None;
   }
  void reset() override {}

  bool finished() const override { return true; }
};

class MockbusCommunicator : public ModbusCommunicator {
public:

  /**
    Entries of tuple have these meanings:
    1: address
    2: function
    3: response number (ACK, NACK, or function number)
    4: reply buffer to send back
  */
  typedef std::vector< std::tuple< uint8_t, uint8_t, uint8_t, std::vector<std::byte>>> RepliesVector;

  MockbusCommunicator(IModbusFrameSubscriber& subscriber, RepliesVector replies) noexcept :
  ModbusCommunicator( subscriber, std::make_unique<DummyFrameFactory>(),
  std::make_unique<DummyFrameParser>() ), m_replies(replies)
    {

  }

  ZenError send(uint8_t address, uint8_t function, gsl::span<const std::byte>) noexcept override {
    // check if we can supply that result
    auto itReply = std::find_if(m_replies.begin(), m_replies.end(),
      [address,function](auto const& entry ){
      return std::get<0>(entry) == address && std::get<1>(entry) == function;
    } );

    if (itReply != m_replies.end()) {
      auto local_subscriber = m_subscriber;
      auto reply_function = std::get<2>(*itReply);
      auto reply_data = std::get<3>(*itReply);
      m_futureReplies.emplace_back(std::async([local_subscriber, address, reply_function, reply_data]()
      {
        std::vector<std::byte> bb = reply_data;
        std::this_thread::sleep_for( std::chrono::milliseconds(100));
        gsl::span<std::byte> byteSpan(bb.data(), bb.size());
        local_subscriber->processReceivedData(address, reply_function, byteSpan );
      }));
    } else {
      spdlog::error("Mock reply for address {} and function {} not found",
        address, function);
    }

    return ZenError_None;
  }

  /** Set Baudrate of IO interface (bit/s) */
  ZenError setBaudRate(unsigned int) noexcept override {
    return ZenError_None;
  }


private:
  const RepliesVector m_replies;
  std::vector<std::future<void>> m_futureReplies;

};

}

#endif
