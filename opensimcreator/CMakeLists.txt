list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ensure the necessary python development headers are available on
# the builder's machine
find_package(Python 3.8 COMPONENTS Interpreter Development.Module Development.SABIModule REQUIRED)

# use `nanobind` to map C++ types to the underlying python library
set(NB_USE_SUBMODULE_DEPS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opensimcreator/nanobind)

# compile the C++ to a native module that the python (wrapping) code
# can call into
nanobind_add_module(
    _opensimcreator_native
    NB_STATIC # STABLE_ABI TODO

    # sources
    opensimcreator/src/_opensimcreator_native.cpp
)
target_link_libraries(_opensimcreator_native PRIVATE OpenSimCreator)

# make the output module directly build into the source python package code,
# so that test suites, developers, etc. can directly start using it (e.g. via
# REPL).
set_target_properties(_opensimcreator_native PROPERTIES
    # > Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append
    # > a per-configuration subdirectory to the specified directory ***unless a generator
    # > expression is used***.
    #     - https://cmake.org/cmake/help/latest/prop_tgt/LIBRARY_OUTPUT_DIRECTORY.html
    LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opensimcreator/
    RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opensimcreator/
)

if(BUILD_TESTING)
    # find `pytest` (+ discovery functions)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindPytest.cmake)

    file(GLOB OSC_PYTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.py")

    # use `pytest` to automatically discover all of the available tests
    pytest_discover_tests(
        _opensimcreator_native
        PYTHON_PATH_PREPEND ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        DEPENDS
            _opensimcreator_native
            ${OSC_PYTEST_SOURCES}
        INCLUDE_FILE_PATH
    )

    unset(OSC_PYTEST_SOURCES)
endif()
