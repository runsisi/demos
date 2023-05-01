set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(
    Qt5 5
    REQUIRED
    COMPONENTS Qml Quick Widgets
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-player-1.0 gstreamer-tag-1.0)

add_executable(
    qt-player
    main.cpp
    qml.qrc
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

add_dependencies(qt-player qmlplayerextension)

include(GNUInstallDirs)
install(
    TARGETS qt-player
    OPTIONAL
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(extension)
