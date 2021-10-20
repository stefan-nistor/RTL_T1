cmake_minimum_required(VERSION 3.20)
project(RTL_T1_C C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c)
add_executable(client client.c)
add_executable(dummy dummy.c)