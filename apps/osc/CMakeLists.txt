# -------------- gather user-facing build cache vars ---------------- #

set(
    OSC_REPO_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator"
    CACHE STRING
    "the internet location of the software's repo - used by in-app links, installers, etc."
)
set(
    OSC_HELP_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator/discussions"
    CACHE STRING
    "the internet location of the software's help page - used by in-app links, installers, etc."
)
set(
    OSC_ORGNAME "cbl"
    CACHE STRING
    "the name of the organization that created the application (affects configuration location)"
)
set(
    OSC_APPNAME "osc"
    CACHE STRING
    "the short name for the application (affects configuration location)"
)
set(
    OSC_LONG_APPNAME "OpenSim Creator"
    CACHE STRING
    "a longer, user-friendly name of the application"
)
set(
    OSC_PACKAGE_NAME "OpenSimCreator"
    CACHE STRING
    "a prefix for the name of the built package (e.g. OSC_PACKAGE_NAME-version.exe)"
)
set(
    OSC_SOFTWARE_DESCRIPTION "A standalone GUI for building OpenSim models"
    CACHE STRING
    "a desription of what the software does (shows in package installers on Debian, for example)"
)
set(
    OSC_AUTHOR "Adam Kewley"
    CACHE STRING
    "the user-facing name of the author (appears in installer, etc.)"
)
set(
    OSC_AUTHOR_EMAIL "contact@adamkewley.com"
    CACHE STRING
    "the user-facing email of the author (appears in 'email support', installer, etc.)"
)

# OpenSimCreatorConfig.h
#
# a configured header file that contains values that are accessible in the source
# code at compile-time
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/osc_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/osc/osc_config.h"
)

add_executable(osc

    # configured by `configure_file`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/osc/osc_config.h"

    # entrypoint (main)
    osc.cpp

    # Windows: also link a resources file (rc)
    #
    # the resources file tells MSVC compiler how to compile non-source resources
    # into the output exe. Specifically, it's used to embed the application icon
    # into the `osc` exe
    $<$<CXX_COMPILER_ID:MSVC>:
        ${CMAKE_CURRENT_SOURCE_DIR}/Windows/resources.rc
    >
)

target_link_libraries(osc PRIVATE

    # so that it uses standard compiler flags etc.
    oscar_compiler_configuration

    # so that it can boot into the main osc codebase
    OpenSimCreator
)

target_include_directories(osc PRIVATE

    # Windows: `resources.rc` uses the include path to find resources
    $<$<CXX_COMPILER_ID:MSVC>:
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    >

    # so that source code can `#include <osc/osc_config.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_options(osc PRIVATE

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # open as a desktop app, not CLI
        /SUBSYSTEM:windows

        # as above, call into `main`
        /ENTRY:mainCRTStartup
    >
)

set_target_properties(osc PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)


# -------------- installation/packaging ------------- #

# set shared CPack variables
set(CPACK_PACKAGE_NAME "${OSC_PACKAGE_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VENDOR "${OSC_AUTHOR}")
set(CPACK_PACKAGE_CONTACT "${OSC_AUTHOR} <${OSC_AUTHOR_EMAIL}>")
set(CPACK_PACKAGE_HOMEPAGE_URL "${OSC_REPO_URL}")
set(CPACK_PACKAGE_DESCRIPTION "${OSC_SOFTWARE_DESCRIPTION}")
set(CPACK_PACKAGE_EXECUTABLES "osc;${OSC_PACKAGE_NAME}")

# `include` platform-specific packaging instructions
if (WIN32)
    include(${CMAKE_CURRENT_SOURCE_DIR}/Windows/Packaging.cmake)
elseif(UNIX AND NOT APPLE)  # i.e. it's Linux
    include(${CMAKE_CURRENT_SOURCE_DIR}/Debian/Packaging.cmake)
elseif(APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/MacOS/Packaging.cmake)
endif()


# -------------- development nice-to-haves ------------- #

# generate a dev-centric `osc.toml`
#
#     - this causes osc to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/osc_dev_config.toml.in" "${CMAKE_BINARY_DIR}/osc.toml")

# for development on Windows, copy all runtime dlls to the exe directory
# (because Windows doesn't have an RPATH)
#
# see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
if (WIN32)
    add_custom_command(
        TARGET osc
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:osc> $<TARGET_RUNTIME_DLLS:osc>
        COMMAND_EXPAND_LISTS
    )
endif()
