cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.5.14 LANGUAGES CXX)


# -------------- gather user-facing build cache vars ---------------- #

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
    OSC_EMSCRIPTEN OFF
    CACHE BOOL
    "enable special build parameters for emscripten (emsdk) - don't try this unless you know what you're doing ;)"
)

# set OSC_BUILD_ID
#
# this is so that errors in the binary can potentially be linked to the source
# that produced the binary later on (user errors, etc.)
if(TRUE)

    if(DEFINED ENV{OSC_BUILD_ID})
        set(OSC_DEFAULT_BUILD_ID "$ENV{OSC_BUILD_ID}")
    else()
        set(OSC_DEFAULT_BUILD_ID "CUSTOM_BUILD")
    endif()

    set(
        OSC_BUILD_ID "${OSC_DEFAULT_BUILD_ID}"
        CACHE STRING
        "a unique identifier for this particular build configuration (handy for bug tracing)"
    )
    unset(OSC_DEFAULT_BUILD_ID)
endif()


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
