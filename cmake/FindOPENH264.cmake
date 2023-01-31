# <PackageName>_ROOT needs cmake 3.12+
cmake_minimum_required(VERSION 3.12)

# put following two lines in master CMakeLists.txt to enable local search:
# cmake_policy(SET CMP0074 NEW)
# set(OPENH264_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/openh264)

find_package(PkgConfig QUIET REQUIRED)
pkg_search_module(PC_OpenH264 openh264 QUIET)

find_path(
    OPENH264_INCLUDE_DIR
    NAMES
        codec_api.h
    HINTS
        ${PC_OpenH264_INCLUDE_DIRS}
        ${OPENH264_ROOT}/include
    PATH_SUFFIXES
        wels
)

find_library(
    OPENH264_LIBRARIES
    NAMES
        "${CMAKE_STATIC_LIBRARY_PREFIX}openh264.${CMAKE_STATIC_LIBRARY_SUFFIX}"
        openh264
    HINTS
        ${PC_OpenH264_LIBRARY_DIRS}
        ${OPENH264_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OPENH264
    DEFAULT_MSG
    OPENH264_INCLUDE_DIR
    OPENH264_LIBRARIES
)

mark_as_advanced(
    OPENH264_INCLUDE_DIR
    OPENH264_LIBRARIES
)

if(OPENH264_FOUND AND NOT (TARGET OPENH264::OPENH264))
    add_library(OPENH264::OPENH264 UNKNOWN IMPORTED)
    set_target_properties(
        OPENH264::OPENH264
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENH264_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${OPENH264_LIBRARIES}"
    )
endif()
