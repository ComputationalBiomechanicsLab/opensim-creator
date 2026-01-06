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

# Configure `osc_config.h` (compile-time configuration)
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

# Make OS-specific modifications for the `osc` target
if(WIN32)
    # On Windows, configure an `osc.toml` that's used when running `osc`
    # from the build directory (runtime configuration when developing)
    #
    # See `packaging/` for the `osc.toml` that's used at install-time
    set(OSC_CONFIG_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resources")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/osc.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)

    # On Windows with MSVC, link a `resources.rc` file.
    #
    # It tells the MSVC compiler how to compile non-source resources into
    # the output exe. Specifically, it's used to embed the application icon
    # into the `osc` exe
    target_sources(osc PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:${CMAKE_CURRENT_SOURCE_DIR}/resources.rc>
    )

    # On Windows with MSVC, indicate it's compiling a desktop application.
    target_link_options(osc PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:

            # Windows: open as a desktop app, not CLI
            /SUBSYSTEM:windows

            # Windows: call into `main`, not `WinMain`
            /ENTRY:mainCRTStartup
        >
    )

    # On Windows, copy all runtime dlls to the exe directory
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
    # On Linux, configure an `osc.toml` that's used when running from the
    # build directory (runtime configuration)
    #
    # See `packaging/` for the installed version
    set(OSC_CONFIG_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resources")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/osc.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)

    # On Linux, set RPATH of `osc` to $ORIGIN/../lib, because unix directory
    # structure usually is:
    #
    #     ${SOME_ROOT}/lib/*.so
    #     ${SOME_ROOT}/bin/osc (executable)
    set_target_properties(osc PROPERTIES INSTALL_RPATH "\$ORIGIN/../lib")
elseif(APPLE)
    # On Apple, resource files (configuration, images, etc.) are
    # considered to be part of the BUILD artifact, not an INSTALL concern,
    # because MacOS bundles are "the resultant executable" and contain
    # everything.

    # On Apple, generate + specify a valid `Info.plist` file for the Mac bundle.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
        @ONLY
    )

    # On Apple, build `osc` it as a MacOSX bundle (enables using BUNDLE variables).
    set_target_properties(osc PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist"
        OUTPUT_NAME "${OSC_LONG_APPNAME}"  # This is what users see in the docking bar etc.
    )

    # On Apple, create a symlink named 'osc' inside the bundle next to the main binary
    add_custom_command(TARGET osc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        "OpenSim Creator"
        "$<TARGET_FILE_DIR:osc>/osc"
    )

    # On Apple, generate an `osc.toml` file inside the bundle __at build time__
    set(OSC_CONFIG_RESOURCES_DIR ".")  # relative to `osc.toml` in the bundle
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/osc.toml.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osc.toml"
        @ONLY
    )
    unset(OSC_CONFIG_RESOURCES_DIR)
    target_sources(osc PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/osc.toml")
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/generated/osc.toml"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
            HEADER_FILE_ONLY TRUE
    )

    # On Apple, copy all resource files into the bundle __at build time__
    # and ensure they have the same directory structure
    file(GLOB_RECURSE OSC_RESOURCE_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/*"
    )
    foreach(_absolute_path ${OSC_RESOURCE_FILES})
        file(RELATIVE_PATH _relative_path "${CMAKE_CURRENT_SOURCE_DIR}/../resources/" ${_absolute_path})
        get_filename_component(_relative_directory "${_relative_path}" DIRECTORY)
        target_sources(osc PRIVATE "${_absolute_path}")
        set_source_files_properties("${_absolute_path}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/${_relative_directory}"
            HEADER_FILE_ONLY TRUE
        )
    endforeach()

    # On Apple, copy the desktop icon into the bundle
    target_sources(osc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/osc.icns")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/osc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        HEADER_FILE_ONLY TRUE
    )
endif()

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
