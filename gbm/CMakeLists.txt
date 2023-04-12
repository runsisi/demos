cmake_minimum_required(VERSION 3.16)
project(drm LANGUAGES CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(drm REQUIRED IMPORTED_TARGET libdrm gbm)

add_executable(gbm-dev gbm.cc)
target_link_libraries(
    gbm-dev
    PRIVATE
    PkgConfig::drm
)
