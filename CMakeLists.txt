cmake_minimum_required(VERSION 3.25)

project(oscar VERSION 0.6.2 LANGUAGES CXX C)

option(BUILD_TESTING                   "Build test suites"                                   ON)
option(OSCAR_BUILD_DEMOS               "Build demo tabs"                                     ON)
option(OSCAR_BUILD_HELLOOSCAR          "Build hellooscar executable"                         ON)
option(OSCAR_USE_STRICT_COMPILER_FLAGS "Add strict(er) compilation flags to oscar's targets" OFF)
option(OSCAR_INSTALL                   "Install oscar targets (library, headers, etc.)"      ON)

if(BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

add_subdirectory(liboscar)
if(OSCAR_BUILD_DEMOS)
    add_subdirectory(liboscar-demos)
endif()
if(OSCAR_BUILD_HELLOOSCAR)
    add_subdirectory(hellooscar)
endif()
