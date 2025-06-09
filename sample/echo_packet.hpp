#pragma once

#include <netp.h>
#include <string>
#include <cstring>
#include <iostream>

// Echo packet implementation
class EchoPacket : public netp::BasicPacket<EchoPacket> {
public:
    EchoPacket() = default;
    explicit EchoPacket(const std::string& message) : message_(message) {}

    const std::string& getMessage() const { return message_; }
    void setMessage(const std::string& message) { message_ = message; }

    // Implementation for BasicPacket
    std::vector<uint8_t> serializeImpl() const {
        // Create a vector with the message data
        std::vector<uint8_t> data;
        if (!message_.empty()) {
            data.resize(message_.size());
            std::memcpy(data.data(), message_.data(), message_.size());
            std::cout << "[EchoPacket] Serialized message '" << message_ << "' to " << data.size() << " bytes" << std::endl;
        } else {
            std::cout << "[EchoPacket] Warning: Serializing empty message" << std::endl;
        }
        return data;
    }

    bool deserializeImpl(const uint8_t* data, size_t length) {
        if (data == nullptr && length > 0) {
            std::cout << "[EchoPacket] Error: Null data pointer with non-zero length" << std::endl;
            return false;
        }
        if (length == 0) {
            std::cout << "[EchoPacket] Warning: Deserializing empty data" << std::endl;
            message_.clear();
            return true;
        }
        message_.assign(reinterpret_cast<const char*>(data), length);
        std::cout << "[EchoPacket] Deserialized message '" << message_ << "' from " << length << " bytes" << std::endl;
        return true;
    }

    size_t getDataSizeImpl() const {
        return message_.size();
    }

private:
    std::string message_;
}; 