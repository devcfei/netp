#pragma once

#include <cstdint>
#include <cstring>

#include <vector>
#include <string>
#include <memory>
#include <netp.h>

namespace netp {

// Command definitions
enum class CustomCommand : uint32_t {
    HELLO = 1,          // Initial handshake
    DATA = 2,           // Regular data transfer
    WARNING = 3,        // Warning response for undefined commands
    KILL = 4,          // Kill notification before termination
    UNDEFINED = 0xFFFFFFFF
};

// Packet header structure
#pragma pack(push, 1)
struct CustomPacketHeader {
    uint32_t length;    // Total packet length including header
    uint32_t id;        // Client ID
    uint32_t cmd;       // Command type
};
#pragma pack(pop)

class CustomPacket : public BasicPacket<CustomPacket> {
public:
    CustomPacket() : header_({0, 0, 0}), data_() {}
    
    // Constructor for creating a packet
    CustomPacket(uint32_t id, CustomCommand cmd) {
        header_.id = id;
        header_.cmd = static_cast<uint32_t>(cmd);
        header_.length = sizeof(CustomPacketHeader);
    }

    // Setters
    void setData(const std::vector<uint8_t>& data) {
        data_ = data;
        header_.length = sizeof(CustomPacketHeader) + data_.size();
    }

    void setId(uint32_t id) { header_.id = id; }
    void setCommand(CustomCommand cmd) { header_.cmd = static_cast<uint32_t>(cmd); }

    // Getters
    uint32_t getId() const { return header_.id; }
    CustomCommand getCommand() const { return static_cast<CustomCommand>(header_.cmd); }
    uint32_t getLength() const { return header_.length; }
    const std::vector<uint8_t>& getData() const { return data_; }

    // Implementation of BasicPacket interface
    std::vector<uint8_t> serializeImpl() const {
        std::vector<uint8_t> buffer(header_.length);
        memcpy(buffer.data(), &header_, sizeof(CustomPacketHeader));
        if (!data_.empty()) {
            memcpy(buffer.data() + sizeof(CustomPacketHeader), data_.data(), data_.size());
        }
        return buffer;
    }

    bool deserializeImpl(const uint8_t* data, size_t length) {
        if (length < sizeof(CustomPacketHeader)) {
            return false;
        }

        memcpy(&header_, data, sizeof(CustomPacketHeader));
        
        if (length != header_.length) {
            return false;
        }

        if (header_.length > sizeof(CustomPacketHeader)) {
            data_.resize(header_.length - sizeof(CustomPacketHeader));
            memcpy(data_.data(), data + sizeof(CustomPacketHeader), data_.size());
        } else {
            data_.clear();
        }

        return true;
    }

    size_t getDataSizeImpl() const {
        return header_.length;
    }

private:
    CustomPacketHeader header_;
    std::vector<uint8_t> data_;
};

} // namespace netp 