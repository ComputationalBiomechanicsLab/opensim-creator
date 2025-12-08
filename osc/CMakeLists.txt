option(OSC_USE_STRICT_COMPILER_FLAGS "Add stricter compilation flags to targets" OFF)
set(
    OSC_REPO_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator"
    CACHE STRING
    "the internet location of the software's repo - used by in-app links, installers, etc."
)
mark_as_advanced(OSC_REPO_URL)
set(
    OSC_DOCS_URL "https://docs.opensimcreator.com"
    CACHE STRING
    "a url for the software's documentation - used by in-app links, installers, etc."
)
mark_as_advanced(OSC_DOCS_URL)
set(
    OSC_HELP_URL "https://github.com/ComputationalBiomechanicsLab/opensim-creator/discussions"
    CACHE STRING
    "the internet location of the software's help page - used by in-app links, installers, etc."
)
mark_as_advanced(OSC_HELP_URL)
set(
    OSC_ORGNAME "cbl"
    CACHE STRING
    "the name of the organization that created the application (affects configuration location)"
)
mark_as_advanced(OSC_ORGNAME)
set(
    OSC_APPNAME "osc"
    CACHE STRING
    "the short name for the application (affects configuration location)"
)
mark_as_advanced(OSC_APPNAME)
set(
    OSC_LONG_APPNAME "OpenSim Creator"
    CACHE STRING
    "a longer, user-friendly name of the application"
)
mark_as_advanced(OSC_LONG_APPNAME)
set(
    OSC_SOFTWARE_DESCRIPTION "A standalone GUI for building OpenSim models"
    CACHE STRING
    "a desription of what the software does (shows in package installers on Debian, for example)"
)
mark_as_advanced(OSC_SOFTWARE_DESCRIPTION)
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
option(OSC_BUNDLE_OSCAR_DEMOS "enable/disable bundling oscar demos with the `osc` application" OFF)

# set OSC_BUILD_ID
if(DEFINED ENV{OSC_BUILD_ID})
    set(OSC_BUILD_ID "$ENV{OSC_BUILD_ID}"
        CACHE STRING
        "a unique identifier for this particular build configuration (handy for bug tracing)"
        FORCE
    )
else()
    set(OSC_BUILD_ID "CUSTOM_BUILD"
        CACHE STRING
        "a unique identifier for this particular build configuration (handy for bug tracing)"
    )
endif()

# generate `osc_config.h`
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/osc_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/osc/osc_config.h"
    @ONLY
)

# setup generic (OS-independent) `osc` executable target
add_executable(osc osc.cpp)
set_target_properties(osc PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(osc PUBLIC cxx_std_23)
if(OSC_USE_STRICT_COMPILER_FLAGS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../libopensimcreator/cmake/osc_strict_compiler_options.cmake)  # OSC_STRICT_COMPILER_OPTIONS
    target_compile_options(osc PRIVATE ${OSC_STRICT_COMPILER_OPTIONS})
endif()
target_link_libraries(osc PRIVATE opensimcreator $<$<BOOL:${OSC_BUNDLE_OSCAR_DEMOS}>:oscar_demos>)
target_include_directories(osc
    # so that `#include <osc/osc_config.h>` works
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/
)

# Make OS-specific modifications to `osc`
if(WIN32)
    target_sources(osc PRIVATE
        # Windows: also link a resources file (rc)
        #
        # the resources file tells MSVC compiler how to compile non-source resources
        # into the output exe. Specifically, it's used to embed the application icon
        # into the `osc` exe
        $<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/resources.rc>
    )
    target_link_options(osc PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:

            # Windows: open as a desktop app, not CLI
            /SUBSYSTEM:windows

            # Windows: call into `main`, not `WinMain`
            /ENTRY:mainCRTStartup
        >
    )

    # for development on Windows, copy all runtime dlls to the exe directory
    # (because Windows doesn't have an RPATH)
    #
    # see: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=runtime#genex:TARGET_RUNTIME_DLLS
    add_custom_command(
        TARGET osc
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:osc> $<TARGET_RUNTIME_DLLS:osc>
        COMMAND_EXPAND_LISTS
    )
elseif(UNIX AND NOT APPLE)
    # On Linux, set RPATH of `osc` to $ORIGIN/../lib, because unix directory
    # structure usually is:
    #
    #     ${SOME_ROOT}/lib/*.so
    #     ${SOME_ROOT}/bin/osc (executable)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
elseif(APPLE)
    # When building `osc` for Apple platforms, build it as a MacOSX bundle (enables using BUNDLE variables).
    set_target_properties(osc PROPERTIES MACOSX_BUNDLE TRUE)

    # Generate + specify a valid `Info.plist` file for the bundle.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
        @ONLY
    )
    set_target_properties(osc PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
    )
endif()

# generate a development-specific `osc.toml`
#
#     - this causes osc to load resources from the current source dir
#
#     - which means that devs can edit shaders, models, icons, etc. and immediately
#       test them without having to run a huge copy/sync operation
#
#     - this config is switched out at install-time to a configuration that loads
#       resources from the (copied) resource directory
if(TRUE)
    set(OSC_CONFIG_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resources")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/../build_resources/osc.toml.in"
        "${CMAKE_BINARY_DIR}/osc.toml"  # CARE: this MUST be CMAKE_BINARY_DIR: multiple executables depend on it
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)
endif()
