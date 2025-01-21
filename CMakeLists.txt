cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.5.18 LANGUAGES CXX C)


set(OSC_BUILD_OPENSIMCREATOR ON
    CACHE BOOL
    "enable/disable building OpenSim Creator (i.e. the OpenSim-dependent part of the codebase)"
)
set(OSC_BUILD_DOCS OFF
    CACHE BOOL
    "enable/disable building the documentation (requires that sphinx-build is available on the PATH)"
)
set(OSC_DISCOVER_TESTS ON
    CACHE BOOL
    "enable/disable automatically running test discovery (IDE integration)"
)


# enable testing in this top-level project, so that `ctest` works when
# (e.g.) IDEs get a list of tests when they use `ctest --show-only`
# to list all tests project-wide
#
# see:
#
# - https://cmake.org/cmake/help/latest/command/enable_testing.html
# - https://developercommunity.visualstudio.com/t/test-explorer-doesnt-detect-tests/482451
# - https://stackoverflow.com/a/67984312
#
# `include(CTest)` declares `BUILD_TESTING`, calls `enable_testing` if `BUILD_TESTING` is
# `true` (default)
include(CTest)

add_subdirectory(liboscar)
add_subdirectory(liboscar-demos)
add_subdirectory(hellooscar)
if (${OSC_BUILD_OPENSIMCREATOR})
    add_subdirectory(libosim libosim SYSTEM)  # uses OpenSim headers
    add_subdirectory(libopensimcreator)
    add_subdirectory(osc)
endif()
if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()
