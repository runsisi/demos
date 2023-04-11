cmake_minimum_required(VERSION 3.16)
project(drm)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_search_module(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(va REQUIRED IMPORTED_TARGET libva libva-drm)
pkg_check_modules(
    ffmpeg REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_executable(drmversion drmversion.cc)
target_link_libraries(
    drmversion
    PRIVATE
    PkgConfig::drm
)

add_executable(
    va-pixfmt
    va_pixfmt.cc
)
target_link_libraries(
    va-pixfmt
    PRIVATE
    PkgConfig::drm
    PkgConfig::va
    PkgConfig::ffmpeg
)
