cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.6.0 LANGUAGES CXX C)

set(OSC_BUILD_OPENSIMCREATOR ON
    CACHE BOOL
    "enable/disable building OpenSim Creator (i.e. the OpenSim-dependent part of the codebase)"
)
set(OSC_BUILD_DOCS OFF
    CACHE BOOL
    "enable/disable building the documentation (requires that sphinx-build is available on the PATH)"
)

# This has to be done at the top-level, so that `ctest --show-only` works for
# all test suites, which is how IDEs typically detect tests.
#
# This is similar to `include(CTest)`, but doesn't create all of the extra testing
# targets (e.g. `Continuous`, `NightlyConfigure`).
option(BUILD_TESTING "enable/disable building test suites" ON)
if (BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(liboscar)
add_subdirectory(liboscar-demos)
add_subdirectory(hellooscar)
if (${OSC_BUILD_OPENSIMCREATOR})
    add_subdirectory(libopensimcreator)
    add_subdirectory(osc)
endif()
if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()
