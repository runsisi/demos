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
