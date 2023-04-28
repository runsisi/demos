set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(sdl)

add_library(
    imgui
    STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_sdl2.cpp
)
target_include_directories(
    imgui
    PUBLIC
    imgui
    imgui/backends
)
target_link_libraries(
    imgui
    PUBLIC
    SDL2::SDL2
)

find_package(Threads REQUIRED)

add_library(asio INTERFACE)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
target_include_directories(asio INTERFACE asio/asio/include)
target_link_libraries(asio INTERFACE Threads::Threads)

export(TARGETS asio
    FILE asio.cmake
    NAMESPACE asio::)

add_library(asio::asio ALIAS asio)

set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(libuv)

unset(MESON_OPTIONS)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    list(APPEND MESON_OPTIONS -Dbuildtype=debug)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    list(APPEND MESON_OPTIONS -Dbuildtype=release)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    list(APPEND MESON_OPTIONS -Dbuildtype=debugoptimized)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    list(APPEND MESON_OPTIONS -Dbuildtype=minsize)
endif()

list(APPEND MESON_OPTIONS -Ddefault_library=static)
list(APPEND MESON_OPTIONS --includedir <INSTALL_DIR>/include)
list(APPEND MESON_OPTIONS --libdir <INSTALL_DIR>/lib)

include(ExternalProject)
ExternalProject_Add(
    aml_ext
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/aml
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/aml
    CONFIGURE_COMMAND meson setup ${MESON_OPTIONS} <BINARY_DIR> <SOURCE_DIR>
    BUILD_COMMAND meson compile -C <BINARY_DIR>
    INSTALL_COMMAND meson install -C <BINARY_DIR>
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libaml.a
)
ExternalProject_Get_Property(aml_ext INSTALL_DIR)

# internal target
add_library(aml_int UNKNOWN IMPORTED)
add_dependencies(aml_int aml_ext)
set_target_properties(
    aml_int PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${INSTALL_DIR}/lib/libaml.a"
)
add_library(aml INTERFACE)
target_compile_definitions(aml INTERFACE AML_UNSTABLE_API=1)
target_include_directories(aml INTERFACE "${INSTALL_DIR}/include")
target_link_libraries(aml INTERFACE aml_int)

add_library(aml::aml ALIAS aml)
