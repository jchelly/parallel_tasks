cmake_minimum_required(VERSION 3.9)

project(parallel_tasks)

find_package(MPI REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)
