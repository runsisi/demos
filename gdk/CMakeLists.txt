cmake_minimum_required(VERSION 3.16)
project(drm LANGUAGES CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(gtk REQUIRED IMPORTED_TARGET gtk+-3.0)

add_executable(gdk gdk.cc)
target_compile_definitions(gdk PRIVATE GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_18)
target_compile_definitions(gdk PRIVATE GDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_18)
target_link_libraries(
    gdk
    PRIVATE
    PkgConfig::gtk
)
