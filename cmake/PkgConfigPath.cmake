#
# Warning: "Cannot generate a safe runtime search path for target" can not be disabled
# see cmake cmOrderDirectories::FindImplicitConflicts()
#

find_package(PkgConfig REQUIRED)

macro(set_pkgconfig_path root)
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${root}")

    list(APPEND PC_DIRS /usr/lib/pkgconfig)
    list(APPEND PC_DIRS /usr/share/pkgconfig)
    if(NOT ${CMAKE_LIBRARY_ARCHITECTURE} STREQUAL "")
        list(APPEND PC_DIRS /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig)
    endif()

    list(TRANSFORM PC_DIRS PREPEND ${root})
    list(JOIN PC_DIRS : PC_DIRS)
    set(ENV{PKG_CONFIG_PATH} "${PC_DIRS}")

    string(REPLACE ":" ";" PC_DIRS ${PC_DIRS})
    list(TRANSFORM PC_DIRS REPLACE ^${root} "")
    list(JOIN PC_DIRS : PC_DIRS)
    set(ENV{PKG_CONFIG_LIBDIR} "${PC_DIRS}")
endmacro()

macro(unset_pkgconfig_path)
    unset(ENV{PKG_CONFIG_SYSROOT_DIR})
    unset(ENV{PKG_CONFIG_PATH})
    unset(ENV{PKG_CONFIG_LIBDIR})
    unset(PC_DIRS)
endmacro()

function(pkg_check_modules root)
    get_filename_component(name "${root}" NAME)

    if(NOT "${root}" STREQUAL "" AND "${root}" STREQUAL "${name}")
        # this is a PkgConfig prefix, not a root path
        _pkg_check_modules(${ARGV})
    else()
        set_pkgconfig_path("${root}")
        _pkg_check_modules(${ARGN})
        unset_pkgconfig_path()
    endif()
endfunction()

function(pkg_search_module root)
    get_filename_component(name "${root}" NAME)

    if(NOT "${root}" STREQUAL "" AND "${root}" STREQUAL "${name}")
        # this is a PkgConfig prefix, not a root path
        _pkg_search_module(${ARGV})
    else()
        set_pkgconfig_path("${root}")
        _pkg_search_module(${ARGN})
        unset_pkgconfig_path()
    endif()
endfunction()
