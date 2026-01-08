if(NOT TARGET osc)
    message(FATAL_ERROR "`osc` must be defined before packaging - it depends on targets/variables set by `osc`")
endif()

set(
    OSC_PACKAGE_NAME "opensimcreator"
    CACHE STRING
    "a prefix for the name of the built package (e.g. OSC_PACKAGE_NAME-version.exe)"
)
mark_as_advanced(OSC_PACKAGE_NAME)
set(
    OSC_AUTHOR "Adam Kewley"
    CACHE STRING
    "the user-facing name of the author (appears in installer, etc.)"
)
mark_as_advanced(OSC_AUTHOR)
set(
    OSC_AUTHOR_EMAIL "contact@adamkewley.com"
    CACHE STRING
    "the user-facing email of the author (appears in 'email support', installer, etc.)"
)
mark_as_advanced(OSC_AUTHOR_EMAIL)
set(
    OSC_SOFTWARE_DESCRIPTION "A standalone GUI for building OpenSim models"
    CACHE STRING
    "a description of what the software does (usually shows in package installers)"
)
mark_as_advanced(OSC_SOFTWARE_DESCRIPTION)
option(OSC_PACKAGE_PORTABLE_ZIP "Enable creating a portable ZIP package" ${WIN32})

# set OSC_PACKAGING_DIR
if(TRUE)
    if(WIN32)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/msi")
    elseif(UNIX AND NOT APPLE)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/deb")
    elseif(APPLE)
        set(OSC_PACKAGING_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/dmg")
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
if(OSC_PACKAGE_PORTABLE_ZIP)  # If requested, also package the install tree into a zip (portable installer)
    list(APPEND CPACK_GENERATOR "ZIP")
endif()

# `include` platform-specific packaging instructions
include(${OSC_PACKAGING_DIR}/Packaging.cmake)
