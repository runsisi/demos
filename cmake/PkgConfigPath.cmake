#
# Warning: Cannot generate a safe runtime search path for target
# see cmake cmOrderDirectories::FindImplicitConflicts()
#

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
