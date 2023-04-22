#
# runsisi AT hust.edu.cn
#

list(APPEND components gstreamer-1.0 gstbase-1.0)
list(APPEND components gstcoreelements)

find_path(
    GSTREAMER_INCLUDE_DIR
    NAMES
        gst/gst.h
    PATH_SUFFIXES
        gstreamer-1.0
)
list(APPEND GSTREAMER_VARS GSTREAMER_INCLUDE_DIR)

foreach(c ${components})
    string(TOUPPER ${c} C)

    find_library(
        ${C}_LIBRARY
        NAMES
            # static gstreamer has too many dependencies!
            # "${CMAKE_STATIC_LIBRARY_PREFIX}${c}${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_SHARED_LIBRARY_PREFIX}${c}${CMAKE_SHARED_LIBRARY_SUFFIX}"
        PATH_SUFFIXES
            gstreamer-1.0
    )

    add_library(gstreamer::${c} UNKNOWN IMPORTED)
    set_target_properties(
        gstreamer::${c}
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE_DIR}"
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${${C}_LIBRARY}"
    )
    list(APPEND GSTREAMER_COMPONENTS gstreamer::${c})
    list(APPEND GSTREAMER_ARCHIVES "${${C}_LIBRARY}")
    list(APPEND GSTREAMER_LIBRARY "${${C}_LIBRARY}")
    list(APPEND GSTREAMER_VARS ${C}_LIBRARY)

    get_filename_component(${C}_LIBRARY_DIR "${${C}_LIBRARY}" DIRECTORY)
    list(APPEND GSTREAMER_LIBRARY_DIRS "${${C}_LIBRARY_DIR}")
endforeach()

list(REMOVE_DUPLICATES GSTREAMER_LIBRARY_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    GStreamer
    DEFAULT_MSG
    ${GSTREAMER_VARS}
)

mark_as_advanced(
    GSTREAMER_COMPONENTS
    GSTREAMER_ARCHIVES
    GSTREAMER_INCLUDE_DIR
    GSTREAMER_LIBRARY
    ${GSTREAMER_VARS}
    GSTREAMER_VARS
    GSTREAMER_LIBRARY_DIRS
)

if(GStreamer_FOUND)
    set(GSTREAMER_INCLUDE_DIRS "${GSTREAMER_INCLUDE_DIR}")
    set(GSTREAMER_LIBRARIES "${GSTREAMER_LIBRARY}")
endif()

if(GStreamer_FOUND AND NOT (TARGET GStreamer::GStreamer))
    add_library(GStreamer::GStreamer INTERFACE IMPORTED)
    add_dependencies(GStreamer::GStreamer ${GSTREAMER_COMPONENTS})
    set_target_properties(
        GStreamer::GStreamer
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
        ${GSTREAMER_INCLUDE_DIRS}
        INTERFACE_LINK_LIBRARIES
        "-Wl,--start-group $<JOIN:${GSTREAMER_ARCHIVES}, > -Wl,--end-group"
    )
endif()
