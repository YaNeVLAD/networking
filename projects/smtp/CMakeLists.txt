cmake_minimum_required(VERSION 3.20)
project(smtp)

add_executable(smtp
        smtp_client.cpp
)

target_link_libraries(smtp
        PRIVATE
        common
)