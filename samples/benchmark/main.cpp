#include "benchmark_packet.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <vector>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#endif

std::mutex cout_mutex;
void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << std::endl;
}

// Check system limits for high concurrency
void checkSystemLimits() {
    log("=== System Limits Check ===");
    
#ifdef _WIN32
    // Windows doesn't have the same limits, but we can check some things
    log("Running on Windows - limits may vary");
#else
    struct rlimit limits;
    if (getrlimit(RLIMIT_NOFILE, &limits) == 0) {
        log("File descriptor limit - Current: " + std::to_string(limits.rlim_cur) + 
            ", Maximum: " + std::to_string(limits.rlim_max));
    }
    
    if (getrlimit(RLIMIT_NPROC, &limits) == 0) {
        log("Process limit - Current: " + std::to_string(limits.rlim_cur) + 
            ", Maximum: " + std::to_string(limits.rlim_max));
    }
#endif
    
    log("=== End System Limits ===");
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

// Statistics for benchmark
struct BenchmarkStats {
    std::atomic<size_t> messages_sent{0};
    std::atomic<size_t> messages_received{0};
    std::atomic<size_t> bytes_sent{0};
    std::atomic<size_t> bytes_received{0};
    std::atomic<size_t> clients_started{0};
    std::atomic<size_t> clients_connected{0};
    std::atomic<size_t> clients_failed{0};
    std::atomic<size_t> total_connection_attempts{0};
    std::atomic<size_t> total_connection_failures{0};
    std::atomic<size_t> clients_completed{0};
    std::atomic<size_t> clients_aborted{0};
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
                  << "\n--- Client Statistics ---\n"
                  << "Clients started: " << clients_started << "\n"
                  << "Clients connected: " << clients_connected << " (" 
                  << (clients_started > 0 ? (clients_connected * 100 / clients_started) : 0) << "%)\n"
                  << "Clients failed: " << clients_failed << " (" 
                  << (clients_started > 0 ? (clients_failed * 100 / clients_started) : 0) << "%)\n"
                  << "Clients completed: " << clients_completed << " (" 
                  << (clients_started > 0 ? (clients_completed * 100 / clients_started) : 0) << "%)\n"
                  << "Clients aborted: " << clients_aborted << " (" 
                  << (clients_started > 0 ? (clients_aborted * 100 / clients_started) : 0) << "%)\n"
                  << "Total connection attempts: " << total_connection_attempts << "\n"
                  << "Total connection failures: " << total_connection_failures << " (" 
                  << (total_connection_attempts > 0 ? (total_connection_failures * 100 / total_connection_attempts) : 0) << "%)\n"
                  << std::endl;
    }
};


void printFinalSummary(const BenchmarkStats& stats);


// Benchmark client implementation
void runBenchmarkClient(const std::string& host, uint16_t port, 
                       size_t min_msg_size, size_t max_msg_size,
                       std::chrono::seconds duration,
                       BenchmarkStats& stats) {
    auto client = netp::createClient();
    bool should_run = true;
    const auto reconnect_delay = std::chrono::seconds(1);  // Reduced delay for faster reconnection
    int reconnect_count = 0;
    const int max_reconnect_attempts = 50;  // Increased limit for high concurrency scenarios
    int successful_connections = 0;
    int failed_connections = 0;
    bool client_connected_at_least_once = false;
    bool client_completed_successfully = false;
    
    // Get client start time for logging
    auto client_start_time = std::chrono::steady_clock::now();
    auto client_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        client_start_time - stats.start_time).count();
    
    // Increment clients started counter
    stats.clients_started++;
    
    auto end_time = std::chrono::steady_clock::now() + duration;
    
    while (should_run && std::chrono::steady_clock::now() < end_time) {
        stats.total_connection_attempts++;
        
        if (client->connect(host, port)) {
            successful_connections++;
            if (!client_connected_at_least_once) {
                client_connected_at_least_once = true;
                stats.clients_connected++;
            }
            log("Benchmark client connected to server (successful connections: " + std::to_string(successful_connections) + ")");
            reconnect_count = 0;  // Reset reconnect counter on successful connection
            
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

            // Set up disconnect handler
            conn->setDisconnectHandler([&should_run]() {
                log("Benchmark client disconnected from server");
            });

            // Wait a moment for connection to stabilize
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            while (std::chrono::steady_clock::now() < end_time && client->isConnected()) {
                std::string msg = generateRandomString(msg_size_dist(gen));
                BenchmarkPacket packet(msg);
                
                if (conn->sendPacket(packet)) {
                    stats.messages_sent++;
                    stats.bytes_sent += msg.size();
                } else {
                    log("Benchmark client failed to send packet, will attempt reconnect");
                    break;
                }
                
                // Small delay to prevent overwhelming the server
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            // Disconnect cleanly before reconnecting
            client->disconnect();
            
        } else {
            reconnect_count++;
            failed_connections++;
            stats.total_connection_failures++;
            
            if (reconnect_count > max_reconnect_attempts) {
                log("Benchmark client exceeded maximum reconnection attempts (" + std::to_string(max_reconnect_attempts) + "), stopping");
                log("Final stats - Successful connections: " + std::to_string(successful_connections) + ", Failed attempts: " + std::to_string(failed_connections));
                break;
            }
            
            log("Benchmark client connection attempt " + std::to_string(reconnect_count) + 
                " failed (total failed: " + std::to_string(failed_connections) + "), retrying in " + std::to_string(reconnect_delay.count()) + " seconds...");
            std::this_thread::sleep_for(reconnect_delay);
        }
    }
    
    client->disconnect();
    
    // Calculate client runtime
    auto client_end_time = std::chrono::steady_clock::now();
    auto client_runtime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        client_end_time - client_start_time).count();
    
    // Determine if client completed successfully or was aborted
    if (std::chrono::steady_clock::now() >= end_time) {
        client_completed_successfully = true;
        stats.clients_completed++;
    } else {
        stats.clients_aborted++;
    }
    
    // If client never connected, mark as failed
    if (!client_connected_at_least_once) {
        stats.clients_failed++;
    }
    
    log("Benchmark client finished - Started at +" + std::to_string(client_start_ms) + "ms, " +
        "Runtime: " + std::to_string(client_runtime_ms) + "ms, " +
        "Successful connections: " + std::to_string(successful_connections) + ", " +
        "Failed attempts: " + std::to_string(failed_connections) + ", " +
        "Status: " + (client_completed_successfully ? "COMPLETED" : "ABORTED"));
}

void runServer() {
    auto server = netp::createServer();
    
    server->setConnectionHandler([](netp::ConnectionPtr conn) {
        std::string remote_addr = conn->getRemoteAddress();
        uint16_t remote_port = conn->getRemotePort();
        log("New connection from " + remote_addr + ":" + std::to_string(remote_port));

        // Set up packet handler - just echo back the packet
        conn->setPacketHandler([conn](const std::vector<uint8_t>& data) {
            BenchmarkPacket packet;
            if (packet.deserialize(data.data(), data.size())) {
                conn->sendPacket(packet);
            }
        });

        // Set up error handler
        conn->setErrorHandler([](const std::string& error) {
            log("Server connection error: " + error);
        });

        // Set up disconnect handler
        conn->setDisconnectHandler([conn]() {
            std::string addr = conn->getRemoteAddress();
            uint16_t port = conn->getRemotePort();
            log("Client disconnected: " + addr + ":" + std::to_string(port));
        });
    });

    server->setErrorHandler([](const std::string& error) {
        log("Server error: " + error);
    });

    // Start server
    if (server->start(12345)) {
        log("Benchmark server started on port 12345");
        log("Server configured with backlog=1024 for high concurrency");
        
        // Keep the server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        log("Failed to start benchmark server");
    }
}

void runBenchmark(size_t num_clients, size_t min_msg_size, size_t max_msg_size, int duration_seconds) {
    BenchmarkStats stats;
    stats.start_time = std::chrono::steady_clock::now();
    
    // Check system limits first
    checkSystemLimits();
    
    std::vector<std::thread> client_threads;
    log("Starting benchmark with " + std::to_string(num_clients) + " clients");
    log("Message size range: " + std::to_string(min_msg_size) + " - " + std::to_string(max_msg_size) + " bytes");
    log("Duration: " + std::to_string(duration_seconds) + " seconds");
    log("Note: Clients start with 100ms delays between them");
    
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
    
    log("All " + std::to_string(num_clients) + " clients started. Monitoring progress...");
    
    // Print stats periodically
    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(duration_seconds);
    int last_completed = 0;
    
    while (std::chrono::steady_clock::now() < end_time) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Show progress
        int currently_completed = stats.clients_completed + stats.clients_aborted;
        if (currently_completed > last_completed) {
            log("Progress: " + std::to_string(currently_completed) + "/" + std::to_string(num_clients) + 
                " clients finished (" + std::to_string(currently_completed * 100 / num_clients) + "%)");
            last_completed = currently_completed;
        }
        
        stats.printStats();
    }
    
    log("Benchmark duration completed. Waiting for remaining clients to finish...");
    
    // Wait for all clients to finish
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    log("All clients finished. Generating final report...");
    
    // Print final stats and summary
    stats.printStats(true);
    printFinalSummary(stats);
}

void printUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " server\n"
              << "  " << program << " client <num_clients> [min_msg_size] [max_msg_size] [duration_seconds]\n"
              << "\nBenchmark parameters:\n"
              << "  num_clients      : Number of concurrent clients (1-1000)\n"
              << "  min_msg_size     : Minimum message size in bytes (default: 16)\n"
              << "  max_msg_size     : Maximum message size in bytes (default: 4092)\n"
              << "  duration_seconds : Test duration in seconds (default: 60)\n"
              << std::endl;
}

void printFinalSummary(const BenchmarkStats& stats) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "FINAL BENCHMARK SUMMARY\n";
    std::cout << std::string(60, '=') << "\n";
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - stats.start_time).count();
    if (duration == 0) duration = 1;
    
    std::cout << "Test Duration: " << duration << " seconds\n\n";
    
    // Performance metrics
    std::cout << "PERFORMANCE METRICS:\n";
    std::cout << "  Messages sent: " << stats.messages_sent << " (" << stats.messages_sent/duration << " msg/s)\n";
    std::cout << "  Messages received: " << stats.messages_received << " (" << stats.messages_received/duration << " msg/s)\n";
    std::cout << "  Bytes sent: " << stats.bytes_sent << " (" << stats.bytes_sent/duration << " B/s)\n";
    std::cout << "  Bytes received: " << stats.bytes_received << " (" << stats.bytes_received/duration << " B/s)\n\n";
    
    // Client statistics
    std::cout << "CLIENT STATISTICS:\n";
    std::cout << "  Clients started: " << stats.clients_started << "\n";
    std::cout << "  Clients connected: " << stats.clients_connected << " (" 
              << (stats.clients_started > 0 ? (stats.clients_connected * 100 / stats.clients_started) : 0) << "%)\n";
    std::cout << "  Clients failed: " << stats.clients_failed << " (" 
              << (stats.clients_started > 0 ? (stats.clients_failed * 100 / stats.clients_started) : 0) << "%)\n";
    std::cout << "  Clients completed: " << stats.clients_completed << " (" 
              << (stats.clients_started > 0 ? (stats.clients_completed * 100 / stats.clients_started) : 0) << "%)\n";
    std::cout << "  Clients aborted: " << stats.clients_aborted << " (" 
              << (stats.clients_started > 0 ? (stats.clients_aborted * 100 / stats.clients_started) : 0) << "%)\n\n";
    
    // Connection statistics
    std::cout << "CONNECTION STATISTICS:\n";
    std::cout << "  Total connection attempts: " << stats.total_connection_attempts << "\n";
    std::cout << "  Total connection failures: " << stats.total_connection_failures << " (" 
              << (stats.total_connection_attempts > 0 ? (stats.total_connection_failures * 100 / stats.total_connection_attempts) : 0) << "%)\n";
    std::cout << "  Connection success rate: " 
              << (stats.total_connection_attempts > 0 ? (100 - (stats.total_connection_failures * 100 / stats.total_connection_attempts)) : 0) << "%\n\n";
    
    // Success indicators
    std::cout << "SUCCESS INDICATORS:\n";
    if (stats.clients_connected == stats.clients_started) {
        std::cout << "  [PASS] All clients connected successfully\n";
    } else {
        std::cout << "  [FAIL] " << (stats.clients_started - stats.clients_connected) << " clients failed to connect\n";
    }
    
    if (stats.clients_completed == stats.clients_started) {
        std::cout << "  [PASS] All clients completed successfully\n";
    } else {
        std::cout << "  [FAIL] " << (stats.clients_started - stats.clients_completed) << " clients were aborted\n";
    }
    
    if (stats.total_connection_failures == 0) {
        std::cout << "  [PASS] No connection failures occurred\n";
    } else {
        std::cout << "  [WARN] " << stats.total_connection_failures << " connection failures occurred\n";
    }
    
    std::cout << std::string(60, '=') << std::endl;
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
            std::cerr << "Invalid mode. Use 'server' or 'client'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 