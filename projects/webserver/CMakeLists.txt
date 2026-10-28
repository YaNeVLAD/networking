cmake_minimum_required(VERSION 3.20)
project(webserver)

add_executable(webserver
        webserver.cpp
)

target_link_libraries(webserver
    PRIVATE
    common
)