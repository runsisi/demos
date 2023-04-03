cmake_minimum_required(VERSION 3.16)

project(
    ffmpeg_debug
    CXX
)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/ffmpeg_debug/cmake")
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})

# change FFMPEG_ROOT to locate ffmpeg libraries
set(FFMPEG_ROOT $ENV{HOME}/working/ubuntu/ffmpeg-5.1.2)
find_package(FFmpeg REQUIRED)

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(sdl2 REQUIRED CONFIG)

find_package(PkgConfig REQUIRED)
pkg_check_modules(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(x11 REQUIRED IMPORTED_TARGET x11 xext xv xfixes xau xdmcp)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb xcb-shm xcb-xfixes xcb-shape)
pkg_check_modules(va REQUIRED IMPORTED_TARGET libva libva-drm libva-x11 libva-wayland)
pkg_check_modules(alsa REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(sndio REQUIRED IMPORTED_TARGET sndio)
pkg_check_modules(pa REQUIRED IMPORTED_TARGET libpulse libpulse-mainloop-glib)
pkg_check_modules(vorbis REQUIRED IMPORTED_TARGET vorbis vorbisenc)
pkg_check_modules(opus REQUIRED IMPORTED_TARGET opus)
pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(lame REQUIRED IMPORTED_TARGET lame)
pkg_check_modules(vdpau REQUIRED IMPORTED_TARGET vdpau)
pkg_check_modules(mpp REQUIRED IMPORTED_TARGET rockchip_mpp)

# create an interface library to make --start-group, --end-group work!!!
add_library(ffmpeg_depends INTERFACE IMPORTED)
target_link_libraries(
    ffmpeg_depends
    INTERFACE
    -Wl,--start-group
    PkgConfig::drm
    PkgConfig::va
    PkgConfig::vdpau
    PkgConfig::alsa
    PkgConfig::sndio
    PkgConfig::pa
    PkgConfig::vorbis
    PkgConfig::opus
    PkgConfig::ogg
    PkgConfig::lame
    PkgConfig::xcb
    PkgConfig::x11
    ZLIB::ZLIB
    BZip2::BZip2
    LibLZMA::LibLZMA
    SDL2::SDL2
    PkgConfig::mpp
    -Wl,--end-group
)

add_subdirectory(ffmpeg_debug)

### using system ffmpeg

find_package(PkgConfig REQUIRED)
# reset to default
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
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

add_executable(system_iterate_codec system_iterate_codec.cc)
target_link_libraries(system_iterate_codec PRIVATE PkgConfig::ffmpeg)
