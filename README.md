# netp
 
packet based communication library based on TCP. 

The netp is a static library which links to [libevent](https://github.com/libevent/libevent)


## Building

Building the project with by mk.cmd, all binaries will be in the `install/<BuildType>/bin` folder

```
./mk.cmd    # Build release version
./mk.cmd -d # Build debug version
```

## Integration

### CMake external project

```cmake
# externel build
include(ExternalProject)

set(CMAKE_ARGS_NETP
    "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/lib/netp"
    )

ExternalProject_Add(netp1.0
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/netp
    GIT_REPOSITORY https://github.com/devcfei/netp.git
    CMAKE_ARGS "${CMAKE_ARGS_NETP}"
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/netp/include
    )

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/netp/lib
    )



add_executable(app ${SRCS})
target_link_libraries(app netp event event_extra event_core ws2_32 iphlpapi)
add_dependencies(app netp1.0)
```

### Manually

After build netp,  include and lib folder set for compiler and linker


## Usage

Refers to samples folder for details.

