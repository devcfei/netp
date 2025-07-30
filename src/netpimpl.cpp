#include <netpimpl.h>
#include <netpp.h>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace netp {
namespace impl {

// PacketFramer implementation
std::vector<std::vector<uint8_t>> PacketFramer::processData(const uint8_t* data, size_t length) {
    std::vector<std::vector<uint8_t>> packets;
    
    // Add new data to buffer
    buffer_.insert(buffer_.end(), data, data + length);
    LOGV("[PacketFramer] Added %zu bytes to buffer, total buffer size: %zu", length, buffer_.size());
    
    // Process complete packets
    while (buffer_.size() >= HEADER_SIZE) {
        // Read header
        PacketHeader header;
        std::memcpy(&header.data_length, buffer_.data(), HEADER_SIZE);
        LOGV("[PacketFramer] Processing packet with total length: %u", header.data_length);
        
        // Check if we have a complete packet
        // Note: header.data_length already includes the header size
        if (buffer_.size() >= header.data_length) {
            LOGV("[PacketFramer] Found complete packet of size %u bytes", header.data_length);
            // Extract packet data (excluding header)
            std::vector<uint8_t> packet_data(
                buffer_.begin() + HEADER_SIZE,
                buffer_.begin() + header.data_length
            );
            packets.push_back(std::move(packet_data));
            
            // Remove processed packet from buffer
            buffer_.erase(buffer_.begin(), buffer_.begin() + header.data_length);
            LOGV("[PacketFramer] Removed processed packet, remaining buffer size: %zu", buffer_.size());
        } else {
            LOGV("[PacketFramer] Incomplete packet: have %zu bytes, need %u bytes", buffer_.size(), header.data_length);
            // Not enough data for complete packet
            break;
        }
    }
    
    return packets;
}

std::vector<uint8_t> PacketFramer::framePacket(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> framed_data(HEADER_SIZE + data.size());
    
    // Write header with total length (header + data)
    uint32_t length = static_cast<uint32_t>(HEADER_SIZE + data.size());
    LOGV("[PacketFramer] Framing packet with total size: %u", length);
    std::memcpy(framed_data.data(), &length, HEADER_SIZE);
    
    // Write data
    std::memcpy(framed_data.data() + HEADER_SIZE, data.data(), data.size());
    LOGV("[PacketFramer] Created framed packet of total size: %zu bytes", framed_data.size());
    
    return framed_data;
}

// Factory functions implementation
std::unique_ptr<Server> createServer() {
    return std::make_unique<ServerImpl>();
}

std::unique_ptr<Client> createClient() {
    return std::make_unique<ClientImpl>();
}

} // namespace impl

// Export factory functions to netp namespace
std::unique_ptr<Server> createServer() {
    return impl::createServer();
}

std::unique_ptr<Client> createClient() {
    return impl::createClient();
}

} // namespace netp 