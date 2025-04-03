cmake_minimum_required(VERSION 3.16)
project(drm)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_search_module(xrandr REQUIRED IMPORTED_TARGET xrandr)
pkg_search_module(x11 REQUIRED IMPORTED_TARGET x11)

add_executable(drmversion drmversion.cc)
target_link_libraries(
    drmversion
    PRIVATE
    PkgConfig::drm
)

add_executable(display_info display_info.cc)
target_link_libraries(
    display_info
    PRIVATE
    PkgConfig::drm
    PkgConfig::xrandr
    PkgConfig::x11
)
