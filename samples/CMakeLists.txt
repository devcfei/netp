cmake_minimum_required(VERSION 3.10)
project(netp-samples)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

add_subdirectory(client)
add_subdirectory(server)