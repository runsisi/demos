cmake_minimum_required(VERSION 3.16)
project(drm)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(drm REQUIRED IMPORTED_TARGET libdrm)

add_executable(drmversion drmversion.cc)
target_link_libraries(
    drmversion
    PRIVATE
    PkgConfig::drm
)
