cmake_minimum_required(VERSION 3.20)
project(networking)

set(CMAKE_CXX_STANDARD 26)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddModule.cmake)

add_subdirectory(common)
add_subdirectory(dns_resolver)
add_subdirectory(smtp)
add_subdirectory(socket)
add_subdirectory(webserver)