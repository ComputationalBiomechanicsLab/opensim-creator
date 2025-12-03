cmake_minimum_required(VERSION 3.25)

project(osc VERSION 0.6.2 LANGUAGES CXX C)

option(BUILD_TESTING            "Build test suites"                                     ON)
option(OSC_BUILD_OPYNSIM_INLINE "Inline the OPynSim build"                              OFF)
option(OSC_TEST_OPYNSIM_INLINE  "Inline the OPynSim test suite"                         OFF)
option(OSC_BUILD_DOCS           "Build OpenSim Creator's documentation"                 OFF)
option(OSC_CONFIGURE_PACKAGING  "Configure the build to support installation/packaging" ON)

if(BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

if(OSC_BUILD_OPYNSIM_INLINE)
    set(OPYN_BUILD_TESTING ${OSC_TEST_OPYNSIM_INLINE})
    set(OPYN_BUILD_PYTHON_BINDINGS OFF)
    set(OPYN_BUILD_DOCS OFF)
    set(OPYN_USE_STRICT_COMPILER_FLAGS OFF)
    set(OPYN_PACKAGE OFF)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/opynsim)
endif()

add_subdirectory(liboscar)
add_subdirectory(liboscar-demos)
add_subdirectory(hellooscar)
add_subdirectory(libopensimcreator)
add_subdirectory(osc)

if(OSC_BUILD_DOCS)
    add_subdirectory(docs)
endif()
if(OSC_CONFIGURE_PACKAGING)
    add_subdirectory(packaging)
endif()
