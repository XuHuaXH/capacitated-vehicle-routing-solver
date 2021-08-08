cmake_minimum_required(VERSION 3.13)
project(cvrp)

set(CMAKE_CXX_STANDARD 20)

add_executable(cvrp GA.cpp)
add_executable(test test.cpp)