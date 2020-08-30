cmake_minimum_required(VERSION 3.17)
project(multihome C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(multihome
        multihome.c
        multihome.h)

install(TARGETS multihome
        RUNTIME DESTINATION bin)