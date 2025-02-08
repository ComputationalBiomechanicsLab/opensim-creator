cmake_minimum_required(VERSION 3.15)

project(osc VERSION 0.5.20 LANGUAGES CXX C)


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
set(OSC_BUILD_PYTHON_BINDINGS ON
    CACHE BOOL
    "enable/disable building python bindings"
)


# note: this has to be done at the top-level, so that `ctest --show-only` works for
#       all test suites, which is how IDEs typically detect tests
include(CTest)  # declares `BUILD_TESTING` and calls `enable_testing` if it's set

add_subdirectory(liboscar)
add_subdirectory(liboscar-demos)
add_subdirectory(hellooscar)
if (${OSC_BUILD_OPENSIMCREATOR})
    add_subdirectory(libopensimcreator)
    add_subdirectory(osc)
    if(${OSC_BUILD_PYTHON_BINDINGS})
        add_subdirectory(opensimcreator)
    endif()
endif()
if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()
