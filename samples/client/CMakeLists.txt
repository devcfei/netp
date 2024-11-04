cmake_minimum_required(VERSION 3.10)
project(client)



# set source code
aux_source_directory(. SRCS)


add_executable(client ${SRCS})
target_link_libraries(client netp)

install(TARGETS client 
    EXPORT clientTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    )
