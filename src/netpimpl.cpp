#include <netpimpl.h>
#include <cstring>
#include <winsock2.h>
#include <iostream>

namespace netp {
namespace impl {

// PacketFramer implementation
std::vector<std::vector<uint8_t>> PacketFramer::processData(const uint8_t* data, size_t length) {
    std::vector<std::vector<uint8_t>> packets;
    
    // Add new data to buffer
    buffer_.insert(buffer_.end(), data, data + length);
    std::cout << "[PacketFramer] Added " << length << " bytes to buffer, total buffer size: " << buffer_.size() << std::endl;
    
    // Process complete packets
    while (buffer_.size() >= HEADER_SIZE) {
        // Read header
        PacketHeader header;
        std::memcpy(&header.data_length, buffer_.data(), HEADER_SIZE);
        std::cout << "[PacketFramer] Processing packet with header length: " << header.data_length << std::endl;
        
        // Check if we have a complete packet
        size_t total_size = HEADER_SIZE + header.data_length;
        if (buffer_.size() >= total_size) {
            std::cout << "[PacketFramer] Found complete packet of size " << total_size << " bytes" << std::endl;
            // Extract packet data
            std::vector<uint8_t> packet_data(
                buffer_.begin() + HEADER_SIZE,
                buffer_.begin() + total_size
            );
            packets.push_back(std::move(packet_data));
            
            // Remove processed packet from buffer
            buffer_.erase(buffer_.begin(), buffer_.begin() + total_size);
            std::cout << "[PacketFramer] Removed processed packet, remaining buffer size: " << buffer_.size() << std::endl;
        } else {
            std::cout << "[PacketFramer] Incomplete packet: have " << buffer_.size() << " bytes, need " << total_size << " bytes" << std::endl;
            // Not enough data for complete packet
            break;
        }
    }
    
    return packets;
}

std::vector<uint8_t> PacketFramer::framePacket(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> framed_data(HEADER_SIZE + data.size());
    
    // Write header
    uint32_t length = static_cast<uint32_t>(data.size());
    std::cout << "[PacketFramer] Framing packet with data size: " << length << std::endl;
    std::memcpy(framed_data.data(), &length, HEADER_SIZE);
    
    // Write data
    std::memcpy(framed_data.data() + HEADER_SIZE, data.data(), data.size());
    std::cout << "[PacketFramer] Created framed packet of total size: " << framed_data.size() << " bytes" << std::endl;
    
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