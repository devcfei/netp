cmake_minimum_required(VERSION 3.10)
project(netp)



# set source code
aux_source_directory(. SRCS)


add_library(netp STATIC ${SRCS})
target_link_libraries(netp ws2_32 iphlpapi event event_extra event_core)
add_dependencies(netp libevent-2.1.12 )

set_target_properties(netp PROPERTIES  
    PUBLIC_HEADER "netp.h;netphlp.h"  
    )  

install(TARGETS netp
        EXPORT netpTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
        )
