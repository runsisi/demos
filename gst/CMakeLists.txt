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

find_package(PkgConfig REQUIRED)

set_pkgconfig_path(${INSTALL_ROOT})
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gmodule-2.0 gobject-2.0 gthread-2.0)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-player-1.0 gstreamer-tag-1.0)
unset_pkgconfig_path()

foreach(d ${gst_LIBRARY_DIRS})
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${d}" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        list(APPEND RPATH_DIRS "${d}")
    endif()
endforeach()

list(TRANSFORM RPATH_DIRS APPEND /gstreamer-1.0)
list(JOIN RPATH_DIRS ":" RPATH_DIRS)

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

find_library(gl GL)

add_executable(h264player h264player.cc)
target_link_libraries(
    h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::gst
    PkgConfig::glib
)
set_target_properties(
    h264player
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)
install(
    TARGETS h264player
    OPTIONAL
    DESTINATION bin
)

add_executable(asio-h264player asio-h264player.cc)
target_link_libraries(
    asio-h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    PkgConfig::gst
    asio::asio
)
set_target_properties(
    asio-h264player
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_executable(asio-h264player2 asio-h264player2.cc)
target_link_libraries(
    asio-h264player2
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    PkgConfig::gst
    asio::asio
)
set_target_properties(
    asio-h264player2
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_executable(uv-h264player uv-h264player.cc)
target_link_libraries(
    uv-h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    PkgConfig::gst
    uv_a
)
set_target_properties(
    uv-h264player
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_executable(aml-h264player aml-h264player.cc)
target_link_libraries(
    aml-h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    PkgConfig::gst
    aml::aml
)
set_target_properties(
    aml-h264player
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_subdirectory(qt)
