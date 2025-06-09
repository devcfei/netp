#pragma once

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <memory>
#include <vector>
#include <cstdint>
#include "../packet.hpp"

namespace netp {
namespace backend {

// Smart pointer types for libevent resources
using EventBasePtr = std::unique_ptr<event_base, decltype(&event_base_free)>;
using BufferEventPtr = std::unique_ptr<bufferevent, decltype(&bufferevent_free)>;

// Helper class for packet framing
class PacketFramer {
public:
    static constexpr size_t HEADER_SIZE = sizeof(PacketHeader);

    // Process incoming data and extract complete packets
    std::vector<std::vector<uint8_t>> processData(const uint8_t* data, size_t length);

    // Frame a packet for sending
    static std::vector<uint8_t> framePacket(const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> buffer_;
};

// Helper functions
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);

} // namespace backend
} // namespace netp 