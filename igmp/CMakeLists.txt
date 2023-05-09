cmake_minimum_required(VERSION 3.16)
project(igmpquery LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_executable(igmpquery igmpquery.c)
add_executable(igmpreport igmpreport.c)
add_executable(igmpreport-ra igmpreport-ra.c)
