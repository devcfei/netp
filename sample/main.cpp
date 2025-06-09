#include "echo_packet.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <vector>
#include <iomanip>

std::mutex cout_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << std::endl;
}

// Generate random string of specified length
std::string generateRandomString(size_t length) {
    static const char charset[] = "0123456789"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

    std::string str(length, 0);
    for (size_t i = 0; i < length; ++i) {
        str[i] = charset[dist(gen)];
    }
    return str;
}

// Statistics for benchmark mode
struct BenchmarkStats {
    std::atomic<size_t> messages_sent{0};
    std::atomic<size_t> messages_received{0};
    std::atomic<size_t> bytes_sent{0};
    std::atomic<size_t> bytes_received{0};
    std::chrono::steady_clock::time_point start_time;
    
    void printStats(bool final = false) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        if (duration == 0) duration = 1; // Avoid division by zero
        
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n=== " << (final ? "Final" : "Current") << " Benchmark Statistics ===\n"
                  << "Duration: " << duration << " seconds\n"
                  << "Messages sent: " << messages_sent << " (" << messages_sent/duration << " msg/s)\n"
                  << "Messages received: " << messages_received << " (" << messages_received/duration << " msg/s)\n"
                  << "Bytes sent: " << bytes_sent << " (" << bytes_sent/duration << " B/s)\n"
                  << "Bytes received: " << bytes_received << " (" << bytes_received/duration << " B/s)\n"
                  << std::endl;
    }
};

// Benchmark client implementation
void runBenchmarkClient(const std::string& host, uint16_t port, 
                       size_t min_msg_size, size_t max_msg_size,
                       std::chrono::seconds duration,
                       BenchmarkStats& stats) {
    auto client = netp::createClient();
    
    if (client->connect(host, port)) {
        log("Benchmark client connected to server");
        
        auto conn = client->getConnection();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> msg_size_dist(min_msg_size, max_msg_size);

        // Set up packet handler
        conn->setPacketHandler([&stats](const std::vector<uint8_t>& data) {
            stats.messages_received++;
            stats.bytes_received += data.size();
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Benchmark client error: " + error);
        });

        auto end_time = std::chrono::steady_clock::now() + duration;
        
        while (std::chrono::steady_clock::now() < end_time && client->isConnected()) {
            std::string msg = generateRandomString(msg_size_dist(gen));
            EchoPacket packet(msg);
            
            if (conn->sendPacket(packet)) {
                stats.messages_sent++;
                stats.bytes_sent += msg.size();
            }
            
            // Small delay to prevent overwhelming the server
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        client->disconnect();
        log("Benchmark client disconnected");
    } else {
        log("Benchmark client failed to connect");
    }
}

void runBenchmark(size_t num_clients, size_t min_msg_size, size_t max_msg_size, int duration_seconds) {
    BenchmarkStats stats;
    stats.start_time = std::chrono::steady_clock::now();
    
    std::vector<std::thread> client_threads;
    log("Starting benchmark with " + std::to_string(num_clients) + " clients");
    
    // Start all clients
    for (size_t i = 0; i < num_clients; ++i) {
        client_threads.emplace_back(runBenchmarkClient,
                                  "127.0.0.1", 12345,
                                  min_msg_size, max_msg_size,
                                  std::chrono::seconds(duration_seconds),
                                  std::ref(stats));
        // Small delay between client starts to prevent connection storm
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Print stats periodically
    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(duration_seconds);
    while (std::chrono::steady_clock::now() < end_time) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        stats.printStats();
    }
    
    // Wait for all clients to finish
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    // Print final stats
    stats.printStats(true);
}

void runServer() {
    // Create and configure server
    auto server = netp::createServer();
    
    server->setConnectionHandler([](netp::ConnectionPtr conn) {
        log("New connection from " + conn->getRemoteAddress() + ":" + std::to_string(conn->getRemotePort()));

        // Set up packet handler
        conn->setPacketHandler([conn](const std::vector<uint8_t>& data) {
            log("Server received raw data of size: " + std::to_string(data.size()));
            
            EchoPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                log("Server received message: " + packet.getMessage());
                
                // Create a new packet for response
                EchoPacket response(packet.getMessage());
                if (!conn->sendPacket(response)) {
                    log("Server failed to send echo response!");
                } else {
                    log("Server sent echo response: " + packet.getMessage());
                }
            } else {
                log("Server failed to deserialize packet! Raw data size: " + std::to_string(data.size()));
            }
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Server connection error: " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([conn]() {
            log("Client disconnected: " + conn->getRemoteAddress() + ":" + std::to_string(conn->getRemotePort()));
        });
    });

    // Set up server error handler
    server->setErrorHandler([](const std::string& error) {
        log("Server error: " + error);
    });

    // Start server
    if (server->start(12345)) {
        log("Server started on port 12345");
        
        // Keep the server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        log("Failed to start server");
    }
}

void runClient() {
    // Create and configure client
    auto client = netp::createClient();
    
    if (client->connect("127.0.0.1", 12345)) {
        log("Connected to server");
        
        auto conn = client->getConnection();

        // Set up packet handler
        conn->setPacketHandler([](const std::vector<uint8_t>& data) {
            log("Client received raw data of size: " + std::to_string(data.size()));
            
            EchoPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                log("Client received echo: " + packet.getMessage());
            } else {
                log("Client failed to deserialize response packet! Raw data size: " + std::to_string(data.size()));
            }
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Client connection error: " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([]() {
            log("Disconnected from server");
        });

        log("Type your messages (type 'quit' to exit):");
        
        std::string input;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input == "quit") {
                log("Closing client...");
                break;
            }
            
            if (!input.empty()) {
                EchoPacket packet(input);
                if (conn->sendPacket(packet)) {
                    log("Client sent: " + input);
                } else {
                    log("Client failed to send: " + input);
                }
            }
            
            // Small delay to prevent flooding
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Clean disconnect
        client->disconnect();
    } else {
        log("Failed to connect to server");
    }
}

void printUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " server\n"
              << "  " << program << " client\n"
              << "  " << program << " benchmark <num_clients> [min_msg_size] [max_msg_size] [duration_seconds]\n"
              << "\nBenchmark parameters:\n"
              << "  num_clients      : Number of concurrent clients (1-1000)\n"
              << "  min_msg_size     : Minimum message size in bytes (default: 16)\n"
              << "  max_msg_size     : Maximum message size in bytes (default: 4092)\n"
              << "  duration_seconds : Test duration in seconds (default: 60)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string mode = argv[1];
    
    try {
        if (mode == "server") {
            runServer();
        } else if (mode == "client") {
            runClient();
        } else if (mode == "benchmark") {
            if (argc < 3) {
                printUsage(argv[0]);
                return 1;
            }
            
            size_t num_clients = std::stoul(argv[2]);
            if (num_clients < 1 || num_clients > 1000) {
                std::cerr << "Number of clients must be between 1 and 1000" << std::endl;
                return 1;
            }
            
            size_t min_msg_size = (argc > 3) ? std::stoul(argv[3]) : 16;
            size_t max_msg_size = (argc > 4) ? std::stoul(argv[4]) : 4092;
            int duration = (argc > 5) ? std::stoi(argv[5]) : 60;
            
            if (min_msg_size > max_msg_size || max_msg_size > 4092) {
                std::cerr << "Invalid message size range. Maximum allowed size is 4092 bytes." << std::endl;
                return 1;
            }
            
            if (duration < 1 || duration > 3600) {
                std::cerr << "Duration must be between 1 and 3600 seconds" << std::endl;
                return 1;
            }
            
            runBenchmark(num_clients, min_msg_size, max_msg_size, duration);
        } else {
            std::cerr << "Invalid mode. Use 'server', 'client', or 'benchmark'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 