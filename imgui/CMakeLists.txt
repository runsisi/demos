cmake_minimum_required(VERSION 3.16)
project(imgui LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

find_library(gl GL)

add_executable(im-gui imgui.cc)
target_link_libraries(
    im-gui
    PRIVATE
    imgui
    ${gl}
)
