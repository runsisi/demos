cmake_minimum_required(VERSION 3.16)

project(
    ffmpeg_debug
    C CXX
)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(WITH_SYSTEM_FFMPEG "Build with system ffmpeg" ON)
if(NOT WITH_SYSTEM_FFMPEG)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_LIBRARY_SUFFIX})

    # change FFMPEG_ROOT to locate ffmpeg libraries
    set(FFMPEG_ROOT "$ENV{HOME}/working/ubuntu/ffmpeg-5.1.2" CACHE STRING "FFmpeg build root")

    if(CMAKE_COMPILER_IS_GNUCXX)
        set(static_linker_flags "-static-libstdc++ -static-libgcc")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${static_linker_flags}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${static_linker_flags}")
        unset(static_linker_flags)
    endif()
endif()

#
# using CMAKE_PREFIX_PATH
#

# change FFMPEG_INSTALL_ROOT to locate ffmpeg libraries
set(FFMPEG_INSTALL_ROOT "$ENV{HOME}/working/arch/built" CACHE STRING "FFmpeg install root")
list(APPEND CMAKE_FIND_ROOT_PATH "${FFMPEG_INSTALL_ROOT}")

# we include avcodec.h using <libavcodec/avcodec.h>, so no need to get XXX_INCLUDE_DIRS
# otherwise it will have conflicts between system time.h and libavutil/time.h

find_path(AVCODEC_INCLUDE_DIRS NAMES avcodec.h PATH_SUFFIXES libavcodec)
get_filename_component(AVCODEC_INCLUDE_DIRS "${AVCODEC_INCLUDE_DIRS}" DIRECTORY)
find_library(AVCODEC_LIBRARIES NAMES avcodec)
find_package_handle_standard_args(AVCODEC DEFAULT_MSG AVCODEC_LIBRARIES AVCODEC_INCLUDE_DIRS)

find_path(AVFILTER_INCLUDE_DIRS NAMES avfilter.h PATH_SUFFIXES libavfilter)
get_filename_component(AVFILTER_INCLUDE_DIRS "${AVFILTER_INCLUDE_DIRS}" DIRECTORY)
find_library(AVFILTER_LIBRARIES NAMES avfilter)
find_package_handle_standard_args(AVFILTER DEFAULT_MSG AVFILTER_LIBRARIES AVFILTER_INCLUDE_DIRS)

find_path(AVFORMAT_INCLUDE_DIRS NAMES avformat.h PATH_SUFFIXES libavformat)
get_filename_component(AVFORMAT_INCLUDE_DIRS "${AVFORMAT_INCLUDE_DIRS}" DIRECTORY)
find_library(AVFORMAT_LIBRARIES NAMES avformat)
find_package_handle_standard_args(AVFORMAT DEFAULT_MSG AVFORMAT_LIBRARIES AVFORMAT_INCLUDE_DIRS)

find_path(AVUTIL_INCLUDE_DIRS NAMES avutil.h PATH_SUFFIXES libavutil)
get_filename_component(AVUTIL_INCLUDE_DIRS "${AVUTIL_INCLUDE_DIRS}" DIRECTORY)
find_library(AVUTIL_LIBRARIES NAMES avutil)
find_package_handle_standard_args(AVUTIL DEFAULT_MSG AVUTIL_LIBRARIES AVUTIL_INCLUDE_DIRS)

find_path(SWSCALE_INCLUDE_DIRS NAMES swscale.h PATH_SUFFIXES libswscale)
get_filename_component(SWSCALE_INCLUDE_DIRS "${SWSCALE_INCLUDE_DIRS}" DIRECTORY)
find_library(SWSCALE_LIBRARIES NAMES swscale)
find_package_handle_standard_args(SWSCALE DEFAULT_MSG SWSCALE_LIBRARIES SWSCALE_INCLUDE_DIRS)

if (AVCODEC_FOUND AND AVFILTER_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
    set(LIBAV_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIRS} ${AVFILTER_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})
    set(LIBAV_LIBRARIES ${AVCODEC_LIBRARIES} ${AVFILTER_LIBRARIES}
        ${AVFORMAT_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})
endif()

list(REMOVE_DUPLICATES LIBAV_INCLUDE_DIRS)

add_executable(
    hw-decode2
    hw_decode.c
)
target_include_directories(
    hw-decode2
    PRIVATE
    ${LIBAV_INCLUDE_DIRS}
)
target_link_libraries(
    hw-decode2
    PRIVATE
    ${LIBAV_LIBRARIES}
    ${m}
    Threads::Threads
)

#
# end using CMAKE_PREFIX_PATH
#

find_package(PkgConfig REQUIRED)
pkg_check_modules(drm REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(x11 REQUIRED IMPORTED_TARGET x11 xext xv xfixes xau xdmcp)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb xcb-shm xcb-xfixes xcb-shape)
pkg_check_modules(va REQUIRED IMPORTED_TARGET libva libva-drm libva-x11 libva-wayland)
find_package(Threads REQUIRED)

if(WITH_SYSTEM_FFMPEG)

    find_package(PkgConfig REQUIRED)
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

    # reset find order to link libm.so
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_STATIC_LIBRARY_SUFFIX})
    # libc & libm should link dynamically
    find_library(m m REQUIRED)

else() # WITH_SYSTEM_FFMPEG

    find_package(FFmpeg REQUIRED)

    foreach(i ${FFMPEG_LIBRARIES})
        get_filename_component(i "${i}" DIRECTORY)
        list(APPEND CMAKE_BUILD_RPATH ${i})
    endforeach()

    list(REMOVE_DUPLICATES CMAKE_BUILD_RPATH)

    foreach(i ${CMAKE_BUILD_RPATH})
        list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${i}" isSystemDir)
        if(NOT "${isSystemDir}" STREQUAL "-1")
            list(REMOVE_ITEM CMAKE_BUILD_RPATH ${i})
        endif()
    endforeach()

    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(LibLZMA REQUIRED)
    find_package(SDL2 REQUIRED CONFIG)

    pkg_check_modules(alsa IMPORTED_TARGET alsa)
    pkg_check_modules(sndio IMPORTED_TARGET sndio)
    pkg_check_modules(pa IMPORTED_TARGET libpulse libpulse-mainloop-glib)
    pkg_check_modules(vorbis IMPORTED_TARGET vorbis vorbisenc)
    pkg_check_modules(opus IMPORTED_TARGET opus)
    pkg_check_modules(ogg IMPORTED_TARGET ogg)
    pkg_check_modules(lame IMPORTED_TARGET lame)
    pkg_check_modules(vdpau IMPORTED_TARGET vdpau)
    pkg_check_modules(mpp IMPORTED_TARGET rockchip_mpp)

    # reset find order to link libm.so
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_STATIC_LIBRARY_SUFFIX})
    # libc & libm should link dynamically
    find_library(m m REQUIRED)

    # create an interface library to make --start-group, --end-group work!!!
    add_library(ffmpeg_depends INTERFACE IMPORTED)
    set(
        deps
        PkgConfig::drm
        PkgConfig::va
        PkgConfig::xcb
        PkgConfig::x11
        ZLIB::ZLIB
        BZip2::BZip2
        LibLZMA::LibLZMA
        Threads::Threads
        ${m}
    )
    if(TARGET SDL2::SDL2)
        list(APPEND deps SDL2::SDL2)
    else()
        list(APPEND deps ${SDL2_LIBRARIES})
    endif()
    foreach(i alsa sndio pa vorbis opus ogg lame vdpau mpp)
        if(${i}_FOUND)
            list(APPEND deps PkgConfig::${i})
        endif()
    endforeach()

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
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        av264
        PRIVATE
        PkgConfig::ffmpeg
        nanobench
        ${m}
        Threads::Threads
    )
else()
    target_link_libraries(
        av264
        PRIVATE
        FFmpeg::FFmpeg
        nanobench
        ffmpeg_depends
    )
endif()

add_executable(
    iterate_codec
    iterate_codec.cc
)
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        iterate_codec
        PRIVATE
        PkgConfig::ffmpeg
        ${m}
        Threads::Threads
    )
else()
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
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        kmsgrab-h264
        PRIVATE
        PkgConfig::ffmpeg
        nanobench
        ${m}
        Threads::Threads
    )
else()
    target_link_libraries(
        kmsgrab-h264
        PRIVATE
        FFmpeg::FFmpeg
        nanobench
        ffmpeg_depends
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
        ${m}
        Threads::Threads
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
        ${m}
        Threads::Threads
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
        ${m}
        Threads::Threads
    )
else()
    target_link_libraries(
        hw-decode
        PRIVATE
        FFmpeg::FFmpeg
        ffmpeg_depends
    )
    set_target_properties(
        hw-decode
        PROPERTIES
        BUILD_RPATH
        "${FFMPEG_LIBRARY_DIRS}"
    )
endif()

add_executable(
    rkmpp-decode
    rkmpp_decode.c
)
if(WITH_SYSTEM_FFMPEG)
    target_link_libraries(
        rkmpp-decode
        PRIVATE
        PkgConfig::ffmpeg
        ${m}
        Threads::Threads
    )
else()
    target_link_libraries(
        rkmpp-decode
        PRIVATE
        FFmpeg::FFmpeg
        ffmpeg_depends
    )
endif()
