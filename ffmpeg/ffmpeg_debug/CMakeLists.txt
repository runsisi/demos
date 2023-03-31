add_subdirectory(third)

add_executable(
    av264
    av264.cc
)
# needed on aarch64
target_compile_definitions(
    av264
    PRIVATE
    __STDC_CONSTANT_MACROS
)
target_link_libraries(
    av264
    PRIVATE
    FFmpeg::FFmpeg
    nanobench
    ffmpeg_depends
)

add_executable(
    iterate_codec
    iterate_codec.cc
)
target_include_directories(
    iterate_codec
    PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)
target_link_libraries(
    iterate_codec
    PRIVATE
    ${FFMPEG_LIBRARIES}
    ffmpeg_depends
)
