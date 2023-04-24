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
