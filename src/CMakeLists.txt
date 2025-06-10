cmake_minimum_required(VERSION 3.10)
project(netp)

# Library source files
set(NETP_SOURCES
    netp.h
    netpimpl.h
    netpimpl.cpp
    connection.cpp
    server.cpp
    client.cpp
)

# Create static library
add_library(netp STATIC ${NETP_SOURCES})

# Set C++ standard
set_target_properties(netp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(netp PUBLIC _WIN32)
endif()

# Include directories
target_include_directories(netp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/libevent/include
)

# Link with libevent
target_link_libraries(netp
    PRIVATE
        event
        event_core
        event_extra
    )

if(WIN32)
target_link_libraries(netp
    PRIVATE
        ws2_32
        iphlpapi
)
endif()



# Installation
install(TARGETS netp
    ARCHIVE DESTINATION lib
)

install(FILES netp.h
    DESTINATION include
)
