include(PkgConfigPath)

add_executable(gst-appsrc appsrc.cc)
target_link_libraries(
    gst-appsrc
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
)

pkg_check_modules(${INSTALL_ROOT} gst-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0)

add_executable(gst-appsrc-c appsrc-c.cc)
target_link_libraries(
    gst-appsrc-c
    PRIVATE
    PkgConfig::gst
    PkgConfig::gst-app
    PkgConfig::glib
)
