cmake_minimum_required(VERSION 3.15)

project(opynsim VERSION 0.0.1 LANGUAGES CXX C)

option(BUILD_TESTING              "Build test suites"                    ON)
option(OPYN_BUILD_PYTHON_BINDINGS "Build Python bindings for libopynsim" ON)
option(OPYN_BUILD_DOCS            "Build documentation"                  ON)

if (BUILD_TESTING)
    # This has to be done at the top-level, so that `ctest --show-only` works for
    # all test suites. It is how IDEs typically detect tests.
    #
    # `include(CTest)` isn't used because we don't want extra targets (e.g. `Continuous`).
    enable_testing()
endif()

add_subdirectory(libopynsim)

if(OPYN_BUILD_DOCS)
    add_subdirectory(docs)
endif()

if(OPYN_BUILD_PYTHON_BINDINGS)
    add_subdirectory(opynsim)
endif()
