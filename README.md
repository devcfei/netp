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

### Linux
```bash
# Using the provided build script
./mk.sh

# Or manually
mkdir build
cd build
cmake ..
cmake --build .
```

### Windows
```bash
# Using the provided build script
mk.cmd

# Or manually
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

## Testing

### Linux
```bash
# Run the comprehensive test suite
./test.sh

# Or run individual tests
./install/Debug/bin/netp_echo server &
./install/Debug/bin/netp_echo client
```

### Windows
```bash
# Run the comprehensive test suite
test.bat

# Or run individual tests
install\Debug\bin\netp_echo.exe server
install\Debug\bin\netp_echo.exe client
```

## GitHub Actions

This project includes GitHub Actions workflows for automated building, testing, and releasing:

### Workflows

1. **Build and Test** (`.github/workflows/build-and-test.yml`)
   - Runs on every push to main/master and pull requests
   - Builds the library on Windows and Linux
   - Runs basic tests to ensure functionality
   - Uploads build artifacts for inspection

2. **Build and Release** (`.github/workflows/build-and-release.yml`)
   - Triggers when you push a tag starting with `v` (e.g., `v1.0.0`)
   - Builds Release and Debug versions for Windows and Linux
   - Creates GitHub releases with downloadable packages
   - Can also be triggered manually via workflow dispatch

3. **Publish Package** (`.github/workflows/publish-package.yml`)
   - Creates package archives for distribution
   - Useful for integration with package managers

### Creating a Release

To create a new release:

1. **Tag-based release:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. **Manual release:**
   - Go to Actions tab in GitHub
   - Select "Build and Release" workflow
   - Click "Run workflow"
   - Enter version number (e.g., 1.0.0)
   - Click "Run workflow"

### Release Artifacts

Each release includes:
- Pre-built libraries for Windows and Linux
- Both Debug and Release builds
- Header files and library files
- Installation instructions

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
