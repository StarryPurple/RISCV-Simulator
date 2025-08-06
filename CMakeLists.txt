cmake_minimum_required(VERSION 3.20)
project(RISC-V-Simulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

include_directories(src/include)

add_executable(code src/main.cpp)