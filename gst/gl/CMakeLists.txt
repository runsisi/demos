include(PkgConfigPath)

pkg_check_modules(GL REQUIRED IMPORTED_TARGET gl)
pkg_check_modules("${INSTALL_ROOT}" GstGL REQUIRED IMPORTED_TARGET gstreamer-gl-1.0)

add_executable(gst-sdlshare sdlshare.c)
target_link_libraries(
    gst-sdlshare
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
    PkgConfig::GL
    SDL2::SDL2
    PkgConfig::GstGL
)
set_target_properties(
    gst-sdlshare
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_executable(gst-sdlshare2 sdlshare2.c)
target_link_libraries(
    gst-sdlshare2
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
    PkgConfig::GL
    SDL2::SDL2
    PkgConfig::GstGL
)
set_target_properties(
    gst-sdlshare2
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)
