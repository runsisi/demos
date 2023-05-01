add_library(
    qmlplayerextension
    SHARED
    imagesample.cpp
    player.cpp
    qgstplayer.cpp
    qplayerextension.cpp
    quickrenderer.cpp
)
target_compile_definitions(
    qmlplayerextension
    PRIVATE
    GST_USE_UNSTABLE_API
)
target_link_libraries(
    qmlplayerextension
    PkgConfig::gst
    PkgConfig::glib
    Qt5::Qml
    Qt5::Quick
    Qt5::Widgets
)

include(GNUInstallDirs)
install(
    TARGETS qmlplayerextension
    OPTIONAL
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}/extension
)
install(
    FILES qmldir DESTINATION ${CMAKE_INSTALL_BINDIR}/extension
)
