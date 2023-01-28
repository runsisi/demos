cmake_minimum_required(VERSION 3.23)
project(vgl)

set(CMAKE_CXX_STANDARD 17)

find_library(egl NAMES EGL)
find_library(glx NAMES GLX)
find_library(ogl NAMES OpenGL)
find_library(drm NAMES drm)
find_library(xcb NAMES xcb)
find_library(xcb-dri3 NAMES xcb-dri3)
find_library(x11 NAMES X11)
find_library(xrender NAMES Xrender)
find_library(xext NAMES Xext)
find_library(fmt NAMES fmt)

add_executable(xcb xcb.cc)
target_include_directories(xcb PRIVATE /usr/include/libdrm)
target_link_libraries(xcb PRIVATE ${drm} ${xcb} ${xcb-dri3})

add_executable(egl egl.cc)
target_include_directories(egl PRIVATE /usr/include/libdrm)
target_link_libraries(egl PRIVATE ${drm} ${egl})

add_executable(eglx11 eglx11.cc)
target_link_libraries(eglx11 PRIVATE ${x11} ${egl})

add_executable(x11simplewin x11simplewin.cc)
target_link_libraries(x11simplewin PRIVATE ${x11})

add_executable(x11event x11event.cc)
target_link_libraries(x11event PRIVATE ${x11})

add_executable(x11mouse x11mouse.cc)
target_link_libraries(x11mouse PRIVATE ${x11})

add_executable(x11win x11win.cc)
target_link_libraries(x11win PRIVATE ${x11})

add_executable(x11winattr x11winattr.cc)
target_link_libraries(x11winattr PRIVATE ${x11} ${fmt} ${xext})

add_executable(x11gl x11gl.cc)
target_link_libraries(x11gl PRIVATE ${x11} ${glx} ${ogl})

add_executable(x11glnewctx x11glnewctx.cc)
target_link_libraries(x11glnewctx PRIVATE ${x11} ${glx} ${ogl})

add_executable(x11glfinal x11glfinal.cc)
target_link_libraries(x11glfinal PRIVATE ${x11} ${glx} ${ogl} ${xrender})

add_executable(eglreadpixels eglreadpixels.cc)
target_link_libraries(eglreadpixels PRIVATE ${egl} ${ogl})

add_executable(x11glreadpixels x11glreadpixels.cc)
target_link_libraries(x11glreadpixels PRIVATE ${x11} ${glx} ${ogl} ${fmt} ${xext})
