if(NOT TARGET osc)
    message(FATAL_ERROR "`osc` must be defined before packaging - it depends on targets/variables set by `osc`")
endif()

set(
    OSC_PACKAGE_NAME "opensimcreator"
    CACHE STRING
    "a prefix for the name of the built package (e.g. OSC_PACKAGE_NAME-version.exe)"
)
mark_as_advanced(OSC_PACKAGE_NAME)

# set OSC_PACKAGING_DIR
if(TRUE)
    if(WIN32)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/Windows")
    elseif(UNIX AND NOT APPLE)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/Debian")
    elseif(APPLE)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/MacOS")
    else()
        set(OSC_PACKAGING_DIR_DEFAULT "UNKNOWN_PACKAGING_METHOD")
    endif()

    set(OSC_PACKAGING_DIR "${OSC_PACKAGING_DIR_DEFAULT}"
        CACHE PATH
        "a path to a directory containing a `Packaging.cmake` script that configures packaging the application for the target platform"
    )
endif()

# set shared/common CPack variables
set(CPACK_PACKAGE_NAME "${OSC_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "${OSC_AUTHOR}")
set(CPACK_PACKAGE_CONTACT "${OSC_AUTHOR} <${OSC_AUTHOR_EMAIL}>")
set(CPACK_PACKAGE_HOMEPAGE_URL "${OSC_REPO_URL}")
set(CPACK_PACKAGE_DESCRIPTION "${OSC_SOFTWARE_DESCRIPTION}")
set(CPACK_PACKAGE_EXECUTABLES "osc;${OSC_LONG_APPNAME}")  # ensure human-readable application name can be searched in start bar etc. (#1079)

# `include` platform-specific packaging instructions
include(${OSC_PACKAGING_DIR}/Packaging.cmake)
