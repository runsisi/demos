include(PkgConfigPath)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(
    Qt5 5
    REQUIRED
    COMPONENTS Qml Quick Widgets
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gmodule-2.0 gobject-2.0 gthread-2.0)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-player-1.0 gstreamer-tag-1.0)

foreach(d ${gst_LIBRARY_DIRS})
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${d}" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        list(APPEND RPATH_DIRS "${d}")
    endif()
endforeach()

list(TRANSFORM RPATH_DIRS APPEND /gstreamer-1.0)
list(JOIN RPATH_DIRS ":" RPATH_DIRS)

add_executable(
    qt-player
    main.cpp
    qml.qrc
    glib_mainloop_on_qeventloop.cpp
)
target_compile_definitions(
    qt-player
    PRIVATE
    GST_USE_UNSTABLE_API
)
target_link_libraries(
    qt-player
    PkgConfig::gst
    PkgConfig::glib
    Qt5::Qml
    Qt5::Quick
    Qt5::Widgets
)
set_target_properties(
    qt-player
    PROPERTIES
    BUILD_RPATH
    "${RPATH_DIRS}"
)

add_dependencies(qt-player qmlplayerextension)

include(GNUInstallDirs)
install(
    TARGETS qt-player
    OPTIONAL
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(extension)
