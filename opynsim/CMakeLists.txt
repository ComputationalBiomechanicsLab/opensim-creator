include(CMakeDependentOption)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(OPYN_BUILD_OPYNSIM_DEBUGGER "Build the opynsim_debugger target" ON)
cmake_dependent_option(OPYN_BUILD_PYTHON_TESTING "Build python test suites" ON BUILD_TESTING OFF)

# ensure the necessary python development headers are available on
# the builder's machine
find_package(
    Python 3.8
    REQUIRED
    COMPONENTS Interpreter Development Development.Module Development.SABIModule
)

# use `nanobind` to map C++ types to the underlying python library
set(OPYN_OLD_CLANG_TIDY_VAL ${CMAKE_CXX_CLANG_TIDY})
set(CMAKE_CXX_CLANG_TIDY ${OPYN_OLD_CLANG_TIDY_VAL})
set(NB_USE_SUBMODULE_DEPS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opynsim/nanobind SYSTEM)
set(CMAKE_CXX_CLANG_TIDY ${OPYN_OLD_CLANG_TIDY_VAL})
unset(OPYN_OLD_CLANG_TIDY_VAL)

# compile the C++ to a native module that the python (wrapping) code
# can call into
nanobind_add_module(
    _opynsim_native
    NB_STATIC # STABLE_ABI TODO

    # sources
    opynsim/src/_opynsim_native.cpp
)
target_link_libraries(_opynsim_native PRIVATE opynsim)

# make the output module directly build into the source python package code,
# so that test suites, developers, etc. can directly start using it (e.g. via
# REPL).
set_target_properties(_opynsim_native PROPERTIES
    # > Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append
    # > a per-configuration subdirectory to the specified directory ***unless a generator
    # > expression is used***.
    #     - https://cmake.org/cmake/help/latest/prop_tgt/LIBRARY_OUTPUT_DIRECTORY.html
    LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
    RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
)

if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    add_subdirectory(debugger)
endif()

if(OPYN_BUILD_PYTHON_TESTING)
    add_subdirectory(tests)
endif()
