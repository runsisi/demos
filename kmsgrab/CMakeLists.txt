cmake_minimum_required(VERSION 3.16)
project(drm LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(kms REQUIRED IMPORTED_TARGET libdrm gbm gl egl)

set(resources fb.vert fb.frag)

add_library(glsl OBJECT)

foreach(r ${resources})
    add_custom_command(
        OUTPUT ${r}.c
        COMMAND xxd -i ${r} ${CMAKE_CURRENT_BINARY_DIR}/${r}.c
        # xxd -i should run w/o absolute path, otherwise the generated
        # variable names are not determinate
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${r}
    )
    # automatically create a dependency to make sure the file is generated before
    # building the target
    target_sources(glsl PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${r}.c)
endforeach()

add_executable(kmsgrab-egl kmsgrab-egl.c)
target_link_libraries(
    kmsgrab-egl
    PRIVATE
    glsl
    PkgConfig::kms
)
