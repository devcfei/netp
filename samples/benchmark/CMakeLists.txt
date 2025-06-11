cmake_minimum_required(VERSION 3.10)
project(netp_benchmark)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32)
endif()

# Sample source files
set(SAMPLE_SOURCES
    main.cpp
)

# Create sample executable
add_executable(netp_benchmark ${SAMPLE_SOURCES})

# Link with netp library
target_link_libraries(netp_benchmark
    PRIVATE
        netp
)

# Installation
install(TARGETS netp_benchmark
    RUNTIME DESTINATION bin
) 