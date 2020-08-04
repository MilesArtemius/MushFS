cmake_minimum_required(VERSION 3.16)
project(MushFS C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE sources file_system/* windows_adapter/* utils/* main.c presets.h)
add_executable(MushFS ${sources})
