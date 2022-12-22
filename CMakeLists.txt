cmake_minimum_required(VERSION 3.24)
project(Networks_Assignment_1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Networks_Assignment_1
        client_side/client.cpp
        main.cpp
        client_side/test.cpp
        client_side/test.h)
