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

pkg_check_modules(gtk IMPORTED_TARGET gtk4)
set(HAVE_GTK ${gtk_FOUND})

add_executable(gst-appsink appsink.cc)
target_link_libraries(
    gst-appsink
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
)
if(HAVE_GTK)
    target_compile_definitions(
        gst-appsink
        PRIVATE
        HAVE_GTK
    )
    target_link_libraries(
        gst-appsink
        PRIVATE
        PkgConfig::gtk
    )
endif()

add_executable(gst-appsink-async appsink-async.cc)
target_link_libraries(
    gst-appsink-async
    PRIVATE
    PkgConfig::gst
    PkgConfig::glib
)
if(HAVE_GTK)
    target_compile_definitions(
        gst-appsink-async
        PRIVATE
        HAVE_GTK
    )
    target_link_libraries(
        gst-appsink-async
        PRIVATE
        PkgConfig::gtk
    )
endif()
