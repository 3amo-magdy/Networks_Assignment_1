cmake_minimum_required(VERSION 3.24)
project(Networks_Assignment_1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Networks_Assignment_1
        client.cpp
        main.cpp
        server.cpp
        test.cpp
        test.h)
