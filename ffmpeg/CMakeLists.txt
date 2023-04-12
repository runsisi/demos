cmake_minimum_required(VERSION 3.16)

project(
    ffmpeg_debug
    CXX
)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})

option(WITH_SYSTEM_FFMPEG "Build with system ffmpeg" ON)
if(NOT WITH_SYSTEM_FFMPEG)
    # change FFMPEG_ROOT to locate ffmpeg libraries
    set(FFMPEG_ROOT "$ENV{HOME}/working/ubuntu/ffmpeg-5.1.1" CACHE STRING "FFmpeg build root")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(x11 REQUIRED IMPORTED_TARGET x11 xext xv xfixes xau xdmcp)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb xcb-shm xcb-xfixes xcb-shape)
pkg_check_modules(va REQUIRED IMPORTED_TARGET libva libva-drm libva-x11 libva-wayland)

if(WITH_SYSTEM_FFMPEG)

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

else() # WITH_SYSTEM_FFMPEG

    find_package(FFmpeg REQUIRED)

    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(LibLZMA REQUIRED)
    find_package(sdl2 REQUIRED CONFIG)

    pkg_check_modules(alsa REQUIRED IMPORTED_TARGET alsa)
    pkg_check_modules(sndio REQUIRED IMPORTED_TARGET sndio)
    pkg_check_modules(pa REQUIRED IMPORTED_TARGET libpulse libpulse-mainloop-glib)
    pkg_check_modules(vorbis REQUIRED IMPORTED_TARGET vorbis vorbisenc)
    pkg_check_modules(opus REQUIRED IMPORTED_TARGET opus)
    pkg_check_modules(ogg REQUIRED IMPORTED_TARGET ogg)
    pkg_check_modules(lame REQUIRED IMPORTED_TARGET lame)
    pkg_check_modules(vdpau REQUIRED IMPORTED_TARGET vdpau)
    pkg_check_modules(mpp IMPORTED_TARGET rockchip_mpp)

    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_STATIC_LIBRARY_SUFFIX})
    find_library(m m REQUIRED)

    # create an interface library to make --start-group, --end-group work!!!
    add_library(ffmpeg_depends INTERFACE IMPORTED)
    set(
        deps
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
        ${m}
    )
    if(mpp_FOUND)
        list(APPEND deps PkgConfig::mpp)
    endif()

    target_link_libraries(
        ffmpeg_depends
        INTERFACE
        -Wl,--start-group
        ${deps}
        -Wl,--end-group
    )

endif()

add_subdirectory(third)

add_executable(
    av264
    av264.cc
)
# needed on aarch64
target_compile_definitions(
    av264
    PRIVATE
    __STDC_CONSTANT_MACROS
)
if(NOT WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        av264
        PRIVATE
        FFmpeg::FFmpeg
        nanobench
        ffmpeg_depends
    )
else()
    target_link_libraries(
        av264
        PRIVATE
        PkgConfig::ffmpeg
        nanobench
    )
endif()

add_executable(
    iterate_codec
    iterate_codec.cc
)
if(NOT WITH_SYSTEM_FFMPEG)
    target_include_directories(
        iterate_codec
        PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
    )
    target_link_libraries(
        iterate_codec
        PRIVATE
        ${FFMPEG_LIBRARIES}
        ffmpeg_depends
    )
else()
    target_link_libraries(
        iterate_codec
        PRIVATE
        PkgConfig::ffmpeg
    )
endif()

add_executable(
    kmsgrab-h264
    kmsgrab-h264.cc
    kmsgrab.cxx
)
include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${drm_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${drm_LIBRARIES})
check_symbol_exists(drmModeGetFB2 "xf86drmMode.h" HAVE_LIBDRM_GETFB2)
if(HAVE_LIBDRM_GETFB2)
    target_compile_definitions(kmsgrab-h264 PRIVATE HAVE_LIBDRM_GETFB2)
endif()
if(NOT WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        kmsgrab-h264
        PRIVATE
        FFmpeg::FFmpeg
        nanobench
        ffmpeg_depends
    )
else()
    target_link_libraries(
        kmsgrab-h264
        PRIVATE
        PkgConfig::ffmpeg
        nanobench
    )
endif()

add_executable(
    va-pixfmt
    va_pixfmt.cc
)
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        va-pixfmt
        PRIVATE
        PkgConfig::drm
        PkgConfig::va
        PkgConfig::ffmpeg
    )
else()
    target_link_libraries(
        va-pixfmt
        PRIVATE
        FFmpeg::FFmpeg
        ffmpeg_depends
    )
endif()

add_executable(
    av-transfer-pixfmt
    av_transfer_pixfmt.cc
)
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        av-transfer-pixfmt
        PRIVATE
        PkgConfig::ffmpeg
    )
else()
    target_link_libraries(
        av-transfer-pixfmt
        PRIVATE
        FFmpeg::FFmpeg
        ffmpeg_depends
    )
endif()

add_executable(
    hw-decode
    hw_decode.c
)
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        hw-decode
        PRIVATE
        PkgConfig::ffmpeg
    )
else()
    target_link_libraries(
        hw-decode
        PRIVATE
        FFmpeg::FFmpeg
        ffmpeg_depends
    )
endif()
