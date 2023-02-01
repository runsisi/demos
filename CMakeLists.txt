cmake_minimum_required(VERSION 3.16)
project(demos)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(gro)
add_subdirectory(gtype)
add_subdirectory(kfprobe)
add_subdirectory(x11gl)
add_subdirectory(gst)
