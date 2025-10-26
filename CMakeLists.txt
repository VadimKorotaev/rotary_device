cmake_minimum_required(VERSION 3.10)
project(rotary_device C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)