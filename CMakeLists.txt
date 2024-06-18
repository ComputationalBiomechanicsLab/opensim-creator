cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.5.12 LANGUAGES CXX)


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
set(OSC_BUILD_OPENSIMCREATOR ON
    CACHE BOOL
    "enable/disable building OpenSim Creator (i.e. the OpenSim-dependent part of the codebase)"
)
set(
    OSC_BUILD_DOCS OFF
    CACHE BOOL
    "enable/disable building the documentation (requires that sphinx-build is available on the PATH)"
)
set(
    OSC_BUILD_BENCHMARKS ON
    CACHE BOOL
    "enable/disable building the benchmark suite (handy for development)"
)
set(
    OSC_EMSCRIPTEN OFF
    CACHE BOOL
    "enable special build parameters for emscripten (emsdk) - don't try this unless you know what you're doing ;)"
)
set(OSC_EXECUTABLE_SUFFIX ${CMAKE_EXECUTABLE_SUFFIX}
    CACHE BOOL
    "global suffix to place on the end of each executable (platform-dependent)"
)

# set OSC_BUILD_ID
#
# this is so that errors in the binary can potentially be linked to the source
# that produced the binary later on (user errors, etc.)
if(TRUE)

    # compute OSC_DEFAULT_BUILD_ID
    if(DEFINED ENV{GITHUB_RUN_ID})
        # build is running inside GitHub Action CI - use that ID
        set(OSC_DEFAULT_BUILD_ID "GITHUB_$ENV{GITHUB_RUN_ID}.$ENV{GITHUB_RUN_NUMBER}")
    else()
        # build is probably running on a dev's machine - generate a random ID
        string(RANDOM RANDOM_ID)
        set(OSC_DEFAULT_BUILD_ID "CUSTOM_${RANDOM_ID}")
        unset(RANDOM_ID)
    endif()

    set(
        OSC_BUILD_ID "${OSC_DEFAULT_BUILD_ID}"
        CACHE STRING
        "a unique identifier for this particular build configuration (handy for bug tracing)"
    )
    unset(OSC_DEFAULT_BUILD_ID)
endif()


# -------------- set/mutate internal vars ---------------- #

# set LINUX
#
# this is based on whether the UNIX variant being used appears to be Linux
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXECUTABLE_SUFFIX ${OSC_EXECUTABLE_SUFFIX})  # note: cannot be set externally

# -------------- setup top-level project state ---------------- #

# enable testing in this top-level project, so that `ctest` works when
# (e.g.) IDEs get a list of tests when they use `ctest --show-only`
# to list all tests project-wide
#
# see:
#
# - https://cmake.org/cmake/help/latest/command/enable_testing.html
# - https://developercommunity.visualstudio.com/t/test-explorer-doesnt-detect-tests/482451
# - https://stackoverflow.com/a/67984312
enable_testing()

# -------------- add/include subdirectories for each part of the project ---------------- #

add_subdirectory(src)
add_subdirectory(apps)
add_subdirectory(tests)

if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()

if(${OSC_BUILD_BENCHMARKS})
    add_subdirectory(benchmarks)
endif()
