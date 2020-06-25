cmake_minimum_required(VERSION 3.16)
project(ex4 C)

set(CMAKE_C_STANDARD 99)

add_executable(osqueue.h osqueue.c test.c threadPool.c threadPool.h)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")