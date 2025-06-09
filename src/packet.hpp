#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace netp {

// Fixed size packet header with 32-bit length field
struct PacketHeader {
    uint32_t data_length;  // Network byte order
};

// Base packet class that users can inherit from
class Packet {
public:
    virtual ~Packet() = default;
    
    // Serialize the packet into raw bytes
    virtual std::vector<uint8_t> serialize() const = 0;
    
    // Deserialize from raw bytes
    virtual bool deserialize(const uint8_t* data, size_t length) = 0;
    
    // Get the size of the packet data (excluding header)
    virtual size_t getDataSize() const = 0;
};

// Helper template for basic packet types
template<typename T>
class BasicPacket : public Packet {
public:
    std::vector<uint8_t> serialize() const override {
        return static_cast<const T*>(this)->serializeImpl();
    }
    
    bool deserialize(const uint8_t* data, size_t length) override {
        return static_cast<T*>(this)->deserializeImpl(data, length);
    }
    
    size_t getDataSize() const override {
        return static_cast<const T*>(this)->getDataSizeImpl();
    }
};

} // namespace netp 