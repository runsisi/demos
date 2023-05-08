cmake_minimum_required(VERSION 3.16)
project(gst)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(PkgConfigPath)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
# use rpath instead of runpath
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--disable-new-dtags")

# cmake options
set(INSTALL_ROOT "$ENV{HOME}/working/arch/built" CACHE STRING "install root")

pkg_check_modules(${INSTALL_ROOT} glib REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gmodule-2.0 gobject-2.0 gthread-2.0)
pkg_check_modules(${INSTALL_ROOT} gst REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-player-1.0 gstreamer-tag-1.0)

foreach(d ${gst_LIBRARY_DIRS})
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${d}" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        list(APPEND RPATH_DIRS "${d}")
    endif()
endforeach()

list(TRANSFORM RPATH_DIRS APPEND /gstreamer-1.0)
list(JOIN RPATH_DIRS ":" RPATH_DIRS)

add_subdirectory(glibloop)
add_subdirectory(qt)

add_executable(factorylist factorylist.cc)
target_link_libraries(
    factorylist
    PRIVATE
    PkgConfig::glib
    PkgConfig::gst
)
set_target_properties(
    factorylist
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_subdirectory(pipeline)
