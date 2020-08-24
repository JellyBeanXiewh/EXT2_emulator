cmake_minimum_required(VERSION 3.10)
project(ext2_emu C)

set(CMAKE_C_STANDARD 99)

LINK_LIBRARIES(m)

add_executable(ext2_emu main.c fs_operation.c fs_operation.h)