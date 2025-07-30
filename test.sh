#!/bin/bash

# NETP Linux Test Script
# Provides similar functionality to test.bat for Linux systems

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}$1${NC}"
}

# Function to cleanup processes
cleanup() {
    print_status "Cleaning up processes..."
    pkill -f "netp_echo" 2>/dev/null
    pkill -f "netp_benchmark" 2>/dev/null
}

# Set up cleanup on script exit
trap cleanup EXIT

# Function to show menu
show_menu() {
    clear
    print_header "NETP Robust Test Menu"
    echo "===================="
    echo "1. Run Regular Client Test"
    echo "2. Run Benchmark Test"
    echo "3. Exit"
    echo
}

# Function to run regular test
run_regular_test() {
    print_status "Starting regular client test..."
    
    # Start the server in background
    print_status "Starting server..."
    ./install/Debug/bin/netp_echo server &
    SERVER_PID=$!
    
    # Wait for server to start
    sleep 2
    
    # Start 10 clients in background
    print_status "Starting 10 clients..."
    for i in {1..10}; do
        ./install/Debug/bin/netp_echo client &
        sleep 1
    done
    
    print_status "All clients started. Press Enter to terminate all processes..."
    read -r
    
    # Kill all processes
    cleanup
    print_status "Regular test completed."
    sleep 3
}

# Function to show benchmark menu
show_benchmark_menu() {
    clear
    print_header "Benchmark Test Menu"
    echo "================="
    echo "1. Run parallel stress test (5x50 clients)"
    echo "2. Run parallel stress test (5x100 clients)"
    echo "3. Run parallel stress test (5x200 clients)"
    echo "4. Run custom parallel stress test"
    echo "5. Return to main menu"
    echo
}

# Function to run stress test
run_stress_test() {
    local num_clients=$1
    local parallel_count=$2
    local duration=$3
    
    print_header "Starting parallel stress test with:"
    echo "- Clients per instance: $num_clients"
    echo "- Parallel instances: $parallel_count"
    echo "- Total clients: $((num_clients * parallel_count))"
    echo "- Duration per round: $duration seconds"
    echo
    
    read -p "Enter number of test rounds (1-100): " iterations
    iterations=${iterations:-1}
    
    if [ "$iterations" -lt 1 ] || [ "$iterations" -gt 100 ]; then
        iterations=1
    fi
    
    print_status "Starting $iterations rounds of stress testing..."
    
    for ((round=1; round<=iterations; round++)); do
        print_status "Running round $round of $iterations"
        echo "-----------------------------------"
        
        # Start server
        ./install/Debug/bin/netp_echo server &
        SERVER_PID=$!
        sleep 2
        
        # Launch multiple benchmark instances in parallel
        for ((instance=1; instance<=parallel_count; instance++)); do
            print_status "Starting benchmark instance $round-$instance"
            ./install/Debug/bin/netp_benchmark client "$num_clients" 16 4092 "$duration" &
            sleep 1
        done
        
        # Wait for the benchmark duration
        print_status "Waiting for round to complete..."
        sleep "$duration"
        
        # Kill all benchmark processes from this round
        print_status "Cleaning up round $round..."
        pkill -f "netp_benchmark" 2>/dev/null
        
        # Add a delay between rounds
        if [ "$round" -ne "$iterations" ]; then
            print_status "Waiting 10 seconds before next round..."
            sleep 10
        fi
    done
    
    print_status "Parallel stress test completed. All rounds finished."
    read -p "Press Enter to continue..."
}

# Function to run benchmark tests
run_benchmark_test() {
    # Start the server in background
    print_status "Starting server..."
    ./install/Debug/bin/netp_echo server &
    SERVER_PID=$!
    sleep 2
    
    while true; do
        show_benchmark_menu
        read -p "Enter your choice (1-5): " bench_choice
        
        case $bench_choice in
            1)
                run_stress_test 50 5 60
                ;;
            2)
                run_stress_test 100 5 60
                ;;
            3)
                run_stress_test 200 5 60
                ;;
            4)
                read -p "Enter number of clients per instance (1-1000): " num_clients
                read -p "Enter number of parallel instances (1-20): " parallel_count
                read -p "Enter duration in seconds (1-3600): " duration
                
                # Validate inputs
                num_clients=${num_clients:-50}
                parallel_count=${parallel_count:-5}
                duration=${duration:-60}
                
                if [ "$num_clients" -lt 1 ] || [ "$num_clients" -gt 1000 ]; then
                    num_clients=50
                fi
                if [ "$parallel_count" -lt 1 ] || [ "$parallel_count" -gt 20 ]; then
                    parallel_count=5
                fi
                if [ "$duration" -lt 1 ] || [ "$duration" -gt 3600 ]; then
                    duration=60
                fi
                
                run_stress_test "$num_clients" "$parallel_count" "$duration"
                ;;
            5)
                cleanup
                return
                ;;
            *)
                print_error "Invalid choice. Please try again."
                sleep 2
                ;;
        esac
    done
}

# Main menu loop
while true; do
    show_menu
    read -p "Enter your choice (1-3): " choice
    
    case $choice in
        1)
            run_regular_test
            ;;
        2)
            run_benchmark_test
            ;;
        3)
            print_status "Exiting..."
            cleanup
            exit 0
            ;;
        *)
            print_error "Invalid choice. Please try again."
            sleep 2
            ;;
    esac
done 