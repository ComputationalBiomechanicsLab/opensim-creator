# Wrapper used to create individual CTest tests from Pytest tests.
cmake_minimum_required(VERSION 3.20...3.30)

if(CMAKE_SCRIPT_MODE_FILE)
    # Initialize content for the CMake test file.
    set(_content "")

    # Convert library and Python paths to native format.
    cmake_path(CONVERT "${LIBRARY_PATH}" TO_NATIVE_PATH_LIST LIBRARY_PATH)
    cmake_path(CONVERT "${PYTHON_PATH}" TO_NATIVE_PATH_LIST PYTHON_PATH)

    # Serialize path values separated by semicolons (required on Windows).
    string(REPLACE [[;]] [[\\;]] LIBRARY_PATH "${LIBRARY_PATH}")
    string(REPLACE [[;]] [[\\;]] PYTHON_PATH "${PYTHON_PATH}")

    # Set up the encoded environment with required paths.
    set(ENCODED_ENVIRONMENT
        "${LIBRARY_ENV_NAME}=${LIBRARY_PATH}"
        "PYTHONPATH=${PYTHON_PATH}"
    )

    # Serialize additional environment variables if any are provided.
    foreach(env ${ENVIRONMENT})
        string(REPLACE [[;]] [[\\;]] env "${env}")
        list(APPEND ENCODED_ENVIRONMENT "${env}")
    endforeach()

    # Macro to create individual tests with optional test properties.
    macro(create_test NAME IDENTIFIER)
        # CAREFUL: some third-party parsers (e.g. test discovery UIs) don't enjoy the NAME, COMMAND modern syntax?
        string(APPEND _content
            "add_test(\"${NAME}\" \"${Python_EXECUTABLE}\" -m pytest \"${IDENTIFIER}\")\n"
        )

        # Prepare the properties for the test, including the environment settings.
        set(args "PROPERTIES ENVIRONMENT [==[${ENCODED_ENVIRONMENT}]==]")

        # Append any additional properties, escaping complex characters if necessary.
        foreach(property ${TEST_PROPERTIES})
            if(property MATCHES "[^-./:a-zA-Z0-9_]")
                string(APPEND args " [==[${property}]==]")
            else()
                string(APPEND args " ${property}")
            endif()
        endforeach()

        # Append the test properties to the content.
        string(APPEND _content "set_tests_properties(\"${NAME}\" ${args})\n")
    endmacro()

    # If tests are bundled together, create a single test group.
    if (BUNDLE_TESTS)
        create_test("${TEST_GROUP_NAME}" "${WORKING_DIRECTORY}")

    else()
        # Set environment variables for collecting tests.
        set(ENV{${LIBRARY_ENV_NAME}} "${LIBRARY_PATH}")
        set(ENV{PYTHONPATH} "${PYTHON_PATH}")
        set(ENV{PYTHONWARNINGS} "ignore")

        # Collect tests.
        execute_process(
            COMMAND "${Python_EXECUTABLE}" -m pytest --collect-only -q --rootdir=${WORKING_DIRECTORY} .
            OUTPUT_VARIABLE _output_lines
            ERROR_VARIABLE _output_lines
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
        )

        # Check for errors during test collection.
        string(REGEX MATCH "=+ ERRORS =+(.*)" _error "${_output_lines}")

        if (_error)
            message(${_error})
            message(FATAL_ERROR "An error occurred during the collection of Python tests.")
        endif()

        # Convert the collected output into a list of lines.
        string(REPLACE [[;]] [[\;]] _output_lines "${_output_lines}")
        string(REPLACE "\n" ";" _output_lines "${_output_lines}")

        # Regex pattern to identify pytest test identifiers.
        set(test_pattern "([^:]+)\.py(::([^:]+))?::([^:]+)")

        # Iterate through each line to identify and process tests.
        foreach(line ${_output_lines})
            string(REGEX MATCHALL ${test_pattern} matching "${line}")

            # Skip lines that are not identified as tests.
            if (NOT matching)
                continue()
            endif()

            # Extract file, class, and function names from the test pattern.
            set(_file ${CMAKE_MATCH_1})
            set(_class ${CMAKE_MATCH_3})
            set(_func ${CMAKE_MATCH_4})

            # Optionally trim parts of the class or function name.
            if (TRIM_FROM_NAME)
                string(REGEX REPLACE "${TRIM_FROM_NAME}" "" _class "${_class}")
                string(REGEX REPLACE "${TRIM_FROM_NAME}" "" _func "${_func}")
            endif()

            # Form the test name using class and function.
            if (_class)
                set(test_name "${_class}.${_func}")
            else()
                set(test_name "${_func}")
            endif()

            # Optionally strip parameter brackets from the test name.
            if (STRIP_PARAM_BRACKETS)
                string(REGEX REPLACE "\\[(.+)\\]$" ".\\1" test_name "${test_name}")
            endif()

            # Optionally include the file path in the test name.
            if (INCLUDE_FILE_PATH)
                cmake_path(CONVERT "${_file}" TO_CMAKE_PATH_LIST _file)
                string(REGEX REPLACE "/" "." _file "${_file}")
                set(test_name "${_file}.${test_name}")
            endif()

            # Optionally trim parts of the full test name.
            if (TRIM_FROM_FULL_NAME)
                string(REGEX REPLACE "${TRIM_FROM_FULL_NAME}" "" test_name "${test_name}")
            endif()

            # Prefix the test name with the test group name.
            # set(test_name "${TEST_GROUP_NAME}.${test_name}")  # OpenSim Creator: don't prefix it, it reads better in VS2020
            set(test_case "${WORKING_DIRECTORY}/${line}")

            # Create the test for CTest.
            create_test("${test_name}" "${test_case}")
        endforeach()

        # Warn if no tests were discovered.
        if(NOT _content)
            message(WARNING "No Python tests have been discovered.")
        endif()
    endif()

    # Write the generated test content to the specified CTest file.
    file(WRITE ${CTEST_FILE} ${_content})
endif()
