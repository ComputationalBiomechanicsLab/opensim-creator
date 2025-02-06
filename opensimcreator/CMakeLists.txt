list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ensure the necessary python development headers are available on
# the builder's machine
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

find_package(Pytest REQUIRED)

# use `nanobind` to map C++ types to the underlying python library
set(NB_USE_SUBMODULE_DEPS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opensimcreator/nanobind)

# compile the C++ to a "core" module that the python (wrapping) code
# can call into
nanobind_add_module(opensimcreator_core src/opensimcreator_core.cpp)
target_link_libraries(opensimcreator_core PRIVATE OpenSimCreator)

# make the output module directly build into the python package, so that
# test suites, developers, etc. can directly start using it (e.g. via REPL)
set_target_properties(opensimcreator_core PROPERTIES
    # > Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append
    # > a per-configuration subdirectory to the specified directory ***unless a generator
    # > expression is used***.
    #     - https://cmake.org/cmake/help/latest/prop_tgt/LIBRARY_OUTPUT_DIRECTORY.html
    LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/src/opensimcreator/
    RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/src/opensimcreator/
)

if(BUILD_TESTING)
    # use `pytest` to automatically discover all of the available tests
    pytest_discover_tests(
        PythonTest
        PYTHON_PATH_PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        DEPENDS opensimcreator_core
        INCLUDE_FILE_PATH
    )
endif()

