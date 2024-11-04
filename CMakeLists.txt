cmake_minimum_required(VERSION 3.10)
project(netp-project)



option(BUILD_SAMPLE "Build Samples" OFF)

# externel build
include(ExternalProject)

set(CMAKE_ARGS_LIBEVENT 
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent"
    "-DEVENT__LIBRARY_TYPE=STATIC"
    "-DEVENT__DISABLE_OPENSSL=TRUE"
    "-DEVENT__DISABLE_SAMPLES=TRUE"
    "-DEVENT__DISABLE_TESTS=TRUE"
    "-DEVENT__DISABLE_BENCHMARK=TRUE"
    "-DEVENT__DISABLE_REGRESS=TRUE"
    )

ExternalProject_Add(libevent-2.1.12
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libevent
    GIT_REPOSITORY https://github.com/libevent/libevent.git
    GIT_TAG release-2.1.12-stable
    CMAKE_ARGS "${CMAKE_ARGS_LIBEVENT}"
    )
   
add_custom_target(libeventcopy ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Copy libevent libaries"  
    )

add_dependencies(libeventcopy libevent-2.1.12)
add_custom_command(TARGET libeventcopy
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent/lib/event.lib ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent/lib/event_core.lib ${CMAKE_INSTALL_PREFIX}/lib
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent/lib/event_extra.lib ${CMAKE_INSTALL_PREFIX}/lib
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent/include  # libevent headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # libevent headers
    )

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libevent/lib
    )

add_subdirectory(src)

if(BUILD_SAMPLE)
add_subdirectory(samples)
endif()
