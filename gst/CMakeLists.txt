cmake_minimum_required(VERSION 3.16)
project(gst)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(glib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.10)

add_executable(factorylist factorylist.cc)
target_link_libraries(
  factorylist
  PRIVATE
  PkgConfig::glib
  PkgConfig::gstreamer
)
