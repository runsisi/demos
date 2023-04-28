cmake_minimum_required(VERSION 3.16)
project(gst)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
# use rpath instead of runpath
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--disable-new-dtags")

# cmake options
set(GSTREAMER_ROOT "$ENV{HOME}/working/ubuntu/built" CACHE STRING "GStreamer build root")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(NOT "${GSTREAMER_ROOT}" STREQUAL "")
    # local gstreamer
    set(CMAKE_FIND_ROOT_PATH ${GSTREAMER_ROOT})
    find_package(GStreamer REQUIRED)
    unset(CMAKE_FIND_ROOT_PATH)
else()
    # system gstreamer
    find_package(GStreamer REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(glib REQUIRED IMPORTED_TARGET glib-2.0)

add_executable(factorylist factorylist.cc)
target_link_libraries(
  factorylist
  PRIVATE
    PkgConfig::glib
    GStreamer::GStreamer
)
set_target_properties(
    factorylist
    PROPERTIES
    BUILD_RPATH
    "${GSTREAMER_LIBRARY_DIRS}"
)

find_library(gl GL)

add_executable(h264player h264player.cc)
target_link_libraries(
    h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    GStreamer::GStreamer
)
set_target_properties(
    h264player
    PROPERTIES
    BUILD_RPATH
    "${GSTREAMER_LIBRARY_DIRS}"
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
    GStreamer::GStreamer
    asio::asio
)
set_target_properties(
    asio-h264player
    PROPERTIES
    BUILD_RPATH
    "${GSTREAMER_LIBRARY_DIRS}"
)

add_executable(asio-h264player2 asio-h264player2.cc)
target_link_libraries(
    asio-h264player2
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    GStreamer::GStreamer
    asio::asio
)
set_target_properties(
    asio-h264player2
    PROPERTIES
    BUILD_RPATH
    "${GSTREAMER_LIBRARY_DIRS}"
)

add_executable(uv-h264player uv-h264player.cc)
target_link_libraries(
    uv-h264player
    PRIVATE
    imgui
    ${gl}
    PkgConfig::glib
    GStreamer::GStreamer
    uv_a
)
set_target_properties(
    uv-h264player
    PROPERTIES
    BUILD_RPATH
    "${GSTREAMER_LIBRARY_DIRS}"
)
