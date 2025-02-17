# Discover required Pytest target.
#
# This module defines the following imported targets:
#     Pytest::Pytest
#
# It also exposes the 'pytest_discover_tests' function, which adds CTest
# test for each Pytest test. The "BUNDLE_PYTHON_TESTS" environment variable
# can be used to run all discovered tests together.
#
# Usage:
#     find_package(Pytest)
#     find_package(Pytest REQUIRED)
#     find_package(Pytest 4.6.11 REQUIRED)
#
# Note:
#     The Pytest_ROOT environment variable or CMake variable can be used to
#     prepend a custom search path.
#     (https://cmake.org/cmake/help/latest/policy/CMP0074.html)

cmake_minimum_required(VERSION 3.20...3.30)

include(FindPackageHandleStandardArgs)

# Find the `pytest` executable (on the PATH, venv, etc.).
find_program(PYTEST_EXECUTABLE NAMES pytest pytest-3)
mark_as_advanced(PYTEST_EXECUTABLE)
if(PYTEST_EXECUTABLE)
    execute_process(
        COMMAND "${PYTEST_EXECUTABLE}" --version
        OUTPUT_VARIABLE _version
        ERROR_VARIABLE _version
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (_version MATCHES "pytest (version )?([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(PYTEST_VERSION "${CMAKE_MATCH_2}")
    endif()
endif()

# Make this script compatible with `find_package`.
find_package_handle_standard_args(
    Pytest
    REQUIRED_VARS
        PYTEST_EXECUTABLE
    VERSION_VAR
        PYTEST_VERSION
    HANDLE_COMPONENTS
    HANDLE_VERSION_RANGE
)

# If `pytest` is found, export relevant targets/scripts.
if (Pytest_FOUND AND NOT TARGET Pytest::Pytest)
    add_executable(Pytest::Pytest IMPORTED)
    set_target_properties(Pytest::Pytest
        PROPERTIES
            IMPORTED_LOCATION "${PYTEST_EXECUTABLE}"
    )

    # Export `pytest_discover_tests`, which discovers pytest tests and exports them
    # to CTest by writing the usual `CTestTestfile.cmake` to the binary directory.
    function(pytest_discover_tests target)
        cmake_parse_arguments(
            PARSE_ARGV 1 "" "STRIP_PARAM_BRACKETS;INCLUDE_FILE_PATH;BUNDLE_TESTS"
            "WORKING_DIRECTORY;TRIM_FROM_NAME;TRIM_FROM_FULL_NAME"
            "LIBRARY_PATH_PREPEND;PYTHON_PATH_PREPEND;ENVIRONMENT;PROPERTIES;DEPENDS"
        )

        # Set platform-specific library path environment variable.
        if (CMAKE_SYSTEM_NAME STREQUAL Windows)
            set(LIBRARY_ENV_NAME PATH)
        elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
            set(LIBRARY_ENV_NAME DYLD_LIBRARY_PATH)
        else()
            set(LIBRARY_ENV_NAME LD_LIBRARY_PATH)
        endif()

        # Convert paths to CMake-friendly format.
        cmake_path(CONVERT "$ENV{${LIBRARY_ENV_NAME}}" TO_CMAKE_PATH_LIST LIBRARY_PATH)
        cmake_path(CONVERT "$ENV{PYTHONPATH}" TO_CMAKE_PATH_LIST PYTHON_PATH)

        # Prepend specified paths to the library and Python paths.
        if (_LIBRARY_PATH_PREPEND)
            list(REVERSE _LIBRARY_PATH_PREPEND)
            foreach (_path ${_LIBRARY_PATH_PREPEND})
                set(LIBRARY_PATH "${_path}" "${LIBRARY_PATH}")
            endforeach()
        endif()

        if (_PYTHON_PATH_PREPEND)
            list(REVERSE _PYTHON_PATH_PREPEND)
            foreach (_path ${_PYTHON_PATH_PREPEND})
                set(PYTHON_PATH "${_path}" "${PYTHON_PATH}")
            endforeach()
        endif()

        # Set default working directory if none is specified.
        if (NOT _WORKING_DIRECTORY)
            set(_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endif()

        get_property(
            has_counter
            TARGET ${target}
            PROPERTY CTEST_DISCOVERED_TEST_COUNTER
            SET
        )
        if(has_counter)
            get_property(
                counter
                TARGET ${target}
                PROPERTY CTEST_DISCOVERED_TEST_COUNTER
            )
            math(EXPR counter "${counter} + 1")
        else()
            set(counter 1)
        endif()
        set_property(
            TARGET ${target}
            PROPERTY CTEST_DISCOVERED_TEST_COUNTER
            ${counter}
        )

        get_filename_component(_WORKING_DIRECTORY "${_WORKING_DIRECTORY}" REALPATH)

        # Override option by environment variable if available.
        if (DEFINED ENV{BUNDLE_PYTHON_TESTS})
            set(_BUNDLE_TESTS $ENV{BUNDLE_PYTHON_TESTS})
        endif()

        # Define file paths for generated CMake include files.
        set(ctest_file_base "${CMAKE_CURRENT_BINARY_DIR}/${target}[${counter}]")
        set(ctest_include_file "${ctest_file_base}_include.cmake")
        set(ctest_tests_file "${ctest_file_base}_tests.cmake")

        add_custom_command(
            #TARGET ${target} POST_BUILD
            OUTPUT "${ctest_tests_file}"
            DEPENDS ${_DEPENDS}
            COMMAND "${CMAKE_COMMAND}"
                -D "PYTEST_EXECUTABLE=${PYTEST_EXECUTABLE}"
                -D "TEST_GROUP_NAME=${target}"
                -D "BUNDLE_TESTS=${_BUNDLE_TESTS}"
                -D "LIBRARY_ENV_NAME=${LIBRARY_ENV_NAME}"
                -D "LIBRARY_PATH=${LIBRARY_PATH}"
                -D "PYTHON_PATH=${PYTHON_PATH}"
                -D "TRIM_FROM_NAME=${_TRIM_FROM_NAME}"
                -D "TRIM_FROM_FULL_NAME=${_TRIM_FROM_FULL_NAME}"
                -D "STRIP_PARAM_BRACKETS=${_STRIP_PARAM_BRACKETS}"
                -D "INCLUDE_FILE_PATH=${_INCLUDE_FILE_PATH}"
                -D "WORKING_DIRECTORY=${_WORKING_DIRECTORY}"
                -D "ENVIRONMENT=${_ENVIRONMENT}"
                -D "TEST_PROPERTIES=${_PROPERTIES}"
                -D "CTEST_FILE=${ctest_tests_file}"
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PytestAddTests.cmake"
            VERBATIM
        )

        file(WRITE "${ctest_include_file}"
            "if(EXISTS \"${ctest_tests_file}\")\n"
            "    include(\"${ctest_tests_file}\")\n"
            "else()\n"
            "    add_test(${target}_NOT_BUILT ${target}_NOT_BUILT)\n"
            "endif()\n"
        )

        # Create a custom target to run the tests.
        add_custom_target(${target}_tests ALL DEPENDS ${ctest_tests_file})

        # Register the include file to be processed for tests.
        set_property(DIRECTORY
            APPEND PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
        )

    endfunction()
endif()
