# ensure the necessary python development headers are available on
# the builder's machine
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

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

add_custom_target(testopensimcreator_core ALL
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/src "${Python_EXECUTABLE}" -m pytest
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    DEPENDS opensimcreator_core
)
