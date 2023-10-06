cmake_minimum_required(VERSION 3.23)
project(posixspawn)

set(CMAKE_C_STANDARD 99)

add_executable(posixspawn posixspawn.c)
add_executable(child child.c)
add_executable(vfork vfork.c)
