cmake_minimum_required(VERSION 3.16)
project(detect-mounts LANGUAGES CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_executable(detect-mounts mounts.cc)
