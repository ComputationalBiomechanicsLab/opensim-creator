list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ensure the necessary python development headers are available on
# the builder's machine
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

find_package(Pytest REQUIRED)

# use `nanobind` to map C++ types to the underlying python library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nanobind)

# compile the C++ to a "core" module that the python (wrapping) code
# can call into
nanobind_add_module(opensimcreator_core src/opensimcreator_core.cpp)
target_link_libraries(opensimcreator_core PRIVATE OpenSimCreator)

# make the output module directly build into the python package, so that
# test suites, developers, etc. can directly start using it (e.g. via REPL)
set_target_properties(opensimcreator_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/opensimcreator
)

# use `pytest` to automatically discover all of the available tests
pytest_discover_tests(
    PythonTest
    PYTHON_PATH_PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    DEPENDS opensimcreator_core
    INCLUDE_FILE_PATH
)
