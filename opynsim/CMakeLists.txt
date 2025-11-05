include(CMakeDependentOption)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
target_link_libraries(_opynsim_native PRIVATE osim)

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

# Create an executable program that uses the python native API to directly call into
# python (embedded-mode) and run `opynsim`'s `__main__`.
#
# This makes it possible to attach a debugger to this executable and directly break
# into native code.
if(TRUE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/opynsim_debugger.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp
        @ONLY
    )
    add_executable(opynsim_debugger ${CMAKE_CURRENT_BINARY_DIR}/generated/opynsim_debugger.cpp)
    target_link_libraries(opynsim_debugger PUBLIC Python::Python)
    add_dependencies(opynsim_debugger _opynsim_native)
endif()

if(OPYN_BUILD_PYTHON_TESTING)
    find_package(Pytest)
    if(NOT Pytest_FOUND)
        message(FATAL_ERROR
            "pytest is not installed in your current environment. You can solve this by either:\n"
            "  - Disabling python tests by setting the cmake cache variable `OPYN_BUILD_PYTHON_TESTING` to `OFF`\n"
            "  - Installing opynsim's test dependencies into the python environment with:\n"
            "        ${Python_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements/test_requirements.txt\n"
        )
    endif()

    file(GLOB OPYN_PYTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.py")
    pytest_discover_tests(
        _opynsim_native
        PYTHON_PATH_PREPEND
            ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
        DEPENDS
            _opynsim_native
            ${OPYN_PYTEST_SOURCES}
        INCLUDE_FILE_PATH
    )
    unset(OPYN_PYTEST_SOURCES)
endif()
