cmake_minimum_required(VERSION 3.10)

project(netp_custom)

add_executable(netp_custom
    main.cpp
)

target_link_libraries(netp_custom
    PRIVATE
        netp
)

target_include_directories(netp_custom
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
) 



# Installation
install(TARGETS netp_custom
    RUNTIME DESTINATION bin
) 