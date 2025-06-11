#pragma once

#include <netp.h>
#include <string>
#include <cstring>
#include <iostream>

// Benchmark packet implementation
class BenchmarkPacket : public netp::BasicPacket<BenchmarkPacket> {
public:
    BenchmarkPacket() = default;
    explicit BenchmarkPacket(const std::string& payload) : payload_(payload) {}

    const std::string& getPayload() const { return payload_; }
    void setPayload(const std::string& payload) { payload_ = payload; }

    // Implementation for BasicPacket
    std::vector<uint8_t> serializeImpl() const {
        std::vector<uint8_t> data;
        if (!payload_.empty()) {
            data.resize(payload_.size());
            std::memcpy(data.data(), payload_.data(), payload_.size());
        }
        return data;
    }

    bool deserializeImpl(const uint8_t* data, size_t length) {
        if (data == nullptr && length > 0) {
            return false;
        }
        if (length == 0) {
            payload_.clear();
            return true;
        }
        payload_.assign(reinterpret_cast<const char*>(data), length);
        return true;
    }

    size_t getDataSizeImpl() const {
        return payload_.size();
    }

private:
    std::string payload_;
}; 