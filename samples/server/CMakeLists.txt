cmake_minimum_required(VERSION 3.10)
project(server)



# set source code
aux_source_directory(. SRCS)


add_executable(server ${SRCS})
target_link_libraries(server netp)

install(TARGETS server 
    EXPORT serverTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    )

