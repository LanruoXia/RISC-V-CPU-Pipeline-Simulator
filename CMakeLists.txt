cmake_minimum_required(VERSION 3.20)

project(riscv-pipeline-simulator)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -Wall")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(
    Simulator
    src/main.cpp
    src/simulator.cpp
    src/memory_manager.cpp
)
