cmake_minimum_required(VERSION 3.20)
project(socket)

add_executable(socket_client
        client.c
)

add_executable(socket_server
        server.c
)

target_link_libraries(socket_client
        PRIVATE
        common
)

target_link_libraries(socket_server
        PRIVATE
        common
)