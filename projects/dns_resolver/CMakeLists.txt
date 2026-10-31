cmake_minimum_required(VERSION 3.20)
project(dns_resolver)

add_executable(dns_resolver
        dns_resolver.cpp

        include/Types.hpp
        include/Client.hpp
        include/Codec.hpp
        include/IterativeResolver.hpp

        src/Client.cpp
        src/Codec.cpp
        src/IterativeResolver.cpp
)

target_link_libraries(dns_resolver
        PRIVATE
        common
)