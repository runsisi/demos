cmake_minimum_required(VERSION 3.24)
project(udp)

set(CMAKE_CXX_STANDARD 17)

add_executable(gso gso.cc)
add_executable(gro gro.cc)
