# netp - Network Packet Communication Library

A modern C++ library for packet-based TCP communication with support for both client and server implementations. The library provides a clean abstraction layer for network communication with built-in support for packet framing and handling.

## Features

- Clean and modern C++ interface (C++17)
- Support for both client and server implementations
- Abstracted backend system (currently supports libevent)
- Built-in packet framing with 32-bit length header
- Automatic handling of TCP sticky packets
- Template-based packet system for easy custom packet creation
- Sample echo server and client implementation

## Requirements

- CMake 3.10 or higher
- C++17 compliant compiler
- libevent 2.1 or higher

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage Example

### Creating a Custom Packet

```cpp
class MyPacket : public netp::BasicPacket<MyPacket> {
public:
    MyPacket() = default;
    explicit MyPacket(const std::string& data) : data_(data) {}

    std::vector<uint8_t> serializeImpl() const {
        return std::vector<uint8_t>(data_.begin(), data_.end());
    }

    bool deserializeImpl(const uint8_t* data, size_t length) {
        data_.assign(reinterpret_cast<const char*>(data), length);
        return true;
    }

    size_t getDataSizeImpl() const {
        return data_.size();
    }

private:
    std::string data_;
};
```

### Server Implementation

```cpp
netp::Server* server = netp::createServer();

server->setConnectionHandler([](netp::ConnectionPtr conn) {
    conn->setPacketHandler([](const std::vector<uint8_t>& data) {
        // Handle received packet
    });
});

server->start(12345);
```

### Client Implementation

```cpp
netp::Client* client = netp::createClient();

client->connect("localhost", 12345);
auto conn = client->getConnection();

conn->setPacketHandler([](const std::vector<uint8_t>& data) {
    // Handle received packet
});

MyPacket packet("Hello, server!");
conn->sendPacket(packet);
```

## Sample Application

The library includes a sample echo server and client implementation. To run the sample:

```bash
# Run the server
./netp_sample server

# In another terminal, run the client
./netp_sample client
```

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
