cmake_minimum_required(VERSION 3.16)
project(probe LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

execute_process(COMMAND uname -r
  OUTPUT_VARIABLE uname_r
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# Module info
add_definitions(-D__KERNEL__ -DMODULE)
include_directories(/lib/modules/${uname_r}/build/include)
include_directories(/lib/modules/${uname_r}/build/arch/x86/include)

add_executable(kprobe kprobe.c)
add_executable(fprobe fprobe.c)
