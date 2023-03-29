cmake_minimum_required(VERSION 3.23)
project(x11)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(xExts REQUIRED IMPORTED_TARGET xdamage)

find_library(x11 NAMES X11)

add_executable(x11 x11.cc)
target_link_libraries(x11 PRIVATE ${x11} PkgConfig::xExts)
