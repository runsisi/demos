cmake_minimum_required(VERSION 3.16)
project(gtype)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0 gobject-2.0)

add_executable(gtype gtype.c)
target_include_directories(gtype PRIVATE /usr/include/glib-2.0)
target_link_libraries(gtype PRIVATE PkgConfig::glib)
