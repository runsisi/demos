add_executable(gst-appsrc appsrc.cc)
target_link_libraries(
    gst-appsrc
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
)
