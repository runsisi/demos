cmake_minimum_required(VERSION 3.10.0)

project(
    QCustomPlot
    LANGUAGES CXX
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)

find_package(
    Qt5 5
    REQUIRED
    COMPONENTS Core Gui Widgets PrintSupport
)
# linking Qt5::PrintSupport in executable target if this is a STATIC library
add_library(
    QCustomPlot
    SHARED
    qcustomplot.cpp
)
set_target_properties(
    QCustomPlot
    PROPERTIES
    PUBLIC_HEADER qcustomplot.h
    OUTPUT_NAME qcustomplot
)
target_compile_definitions(
    QCustomPlot
    PUBLIC
    QCUSTOMPLOT_COMPILE_LIBRARY
)
target_link_libraries(
    QCustomPlot
    PUBLIC
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport
)

include(GNUInstallDirs)
install(
    TARGETS QCustomPlot
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
