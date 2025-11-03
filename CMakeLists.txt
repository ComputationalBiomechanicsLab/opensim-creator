cmake_minimum_required(VERSION 3.25)

project(osc VERSION 0.6.2 LANGUAGES CXX C)

option(BUILD_TESTING            "Build test suites"                                     ON)
option(OSC_BUILD_OPENSIMCREATOR "Build OpenSim Creator"                                 ON)
option(OSC_BUILD_DOCS           "Build OpenSim Creator's documentation"                 OFF)
option(OSC_CONFIGURE_PACKAGING  "Configure the build to support installation/packaging" ON)

if(BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

add_subdirectory(liboscar)
add_subdirectory(liboscar-demos)
add_subdirectory(hellooscar)
if(${OSC_BUILD_OPENSIMCREATOR})
    add_subdirectory(libopensimcreator)
    add_subdirectory(osc)
    if(${OSC_CONFIGURE_PACKAGING})
        add_subdirectory(packaging)
    endif()
endif()
if(${OSC_BUILD_DOCS})
    add_subdirectory(docs)
endif()
