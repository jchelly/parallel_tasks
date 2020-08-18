cmake_minimum_required(VERSION 3.9)

project(parallel_tasks)

find_package(MPI REQUIRED)

add_subdirectory(src)
