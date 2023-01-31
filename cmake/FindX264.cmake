# <PackageName>_ROOT needs cmake 3.12+
cmake_minimum_required(VERSION 3.12)

# put following two lines in master CMakeLists.txt to enable local search:
# cmake_policy(SET CMP0074 NEW)
# set(OPENH264_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/x264)

find_path(
    X264_INCLUDE_DIR
    NAMES
        x264.h
    HINTS
        ${X264_ROOT}/include
)

find_library(
    X264_LIBRARIES
    NAMES
        "${CMAKE_STATIC_LIBRARY_PREFIX}x264.${CMAKE_STATIC_LIBRARY_SUFFIX}"
        x264
    HINTS
        ${X264_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    X264
    DEFAULT_MSG
    X264_INCLUDE_DIR
    X264_LIBRARIES
)

mark_as_advanced(
    X264_INCLUDE_DIR
    X264_LIBRARIES
)

if(X264_FOUND AND NOT (TARGET X264::X264))
    add_library(X264::X264 UNKNOWN IMPORTED)
    set_target_properties(
        X264::X264
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${X264_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${X264_LIBRARIES}"
    )
endif()
