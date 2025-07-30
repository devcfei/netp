cmake_minimum_required(VERSION 3.10)
project(reconnection_test)

# Create executable
add_executable(reconnection_test main.cpp)

# Set C++ standard
set_target_properties(reconnection_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Link with netp library
target_link_libraries(reconnection_test netp)

# Windows-specific settings
if(WIN32)
    target_link_libraries(reconnection_test ws2_32 iphlpapi)
endif() 