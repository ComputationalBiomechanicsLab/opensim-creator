if(NOT TARGET osc)
    message(FATAL_ERROR "`osc` must be defined before packaging - it depends on targets/variables set by `osc`")
endif()

option(OSC_CODESIGN_ENABLED     "Enable codesigning the package (if applicable)" OFF)
option(OSC_NOTARIZATION_ENABLED "Enable notarizing the package (if applicable)"  OFF)

# Set standard (shared) CPack variables
set(CPACK_PACKAGE_NAME                "opensimcreator")
set(CPACK_PACKAGE_VENDOR              "Adam Kewley")
set(CPACK_PACKAGE_CONTACT             "Adam Kewley <contact@adamkewley.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL        "${OSC_REPO_URL}")
set(CPACK_PACKAGE_DESCRIPTION         "A standalone GUI for building OpenSim models")
set(CPACK_PACKAGE_EXECUTABLES         "osc;${OSC_LONG_APPNAME}")  # ensure human-readable application name can be searched in start bar etc. (#1079)

# Set custom CPack variables that are forwarded to `CPackOptions.cmake` scripts
set(CPACK_OSC_CODESIGN_ENABLED        ${OSC_CODESIGN_ENABLED})
set(CPACK_OSC_NOTARIZATION_ENABLED    ${OSC_NOTARIZATION_ENABLED})
set(CPACK_OSC_LONG_APPNAME            "${OSC_LONG_APPNAME}")
set(CPACK_OSC_PROJECT_NAME            "${PROJECT_NAME}")
set(CPACK_OSC_PROJECT_VERSION         "${PROJECT_VERSION}")
set(CPACK_OSC_PROJECT_SOURCE_DIR      "${PROJECT_SOURCE_DIR}")
set(CPACK_OSC_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
set(CPACK_OSC_CMAKE_SYSTEM_PROCESSOR  "${CMAKE_SYSTEM_PROCESSOR}")

# Specify the top-level `CPackOptions.cmake` script
set(CPACK_PROJECT_CONFIG_FILE      "${CMAKE_CURRENT_SOURCE_DIR}/CPackOptions.cmake")

# Figure out `CPACK_GENERATOR` defaults
if(WIN32)
    set(CPACK_GENERATOR "ZIP;WIX")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

# Now that all `CPACK_` variables are set, include CPack
include(CPack)
