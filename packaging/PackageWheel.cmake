# It can be useful to enable this if there's packaging problems
if(CPACK_WHEEL_DEBUG)
    include(CMakePrintHelpers)
    cmake_print_variables(CPACK_TEMPORARY_DIRECTORY)
    cmake_print_variables(CPACK_WHEEL_PYTHON_EXECUTABLE)
    cmake_print_variables(CPACK_TOPLEVEL_DIRECTORY)
    cmake_print_variables(CPACK_PACKAGE_DIRECTORY)
    cmake_print_variables(CPACK_PACKAGE_FILE_NAME)
    cmake_print_variables(CPACK_WHEEL_PACKAGING_SOURCE_DIR)
    cmake_print_variables(CPACK_WHEEL_METADATA_FILE)
    cmake_print_variables(CPACK_WHEEL_WHEEL_FILE)
    cmake_print_variables(CPACK_WHEEL_NAME)
    cmake_print_variables(CPACK_WHEEL_VERSION)
    cmake_print_variables(CPACK_WHEEL_TAG)
    cmake_print_variables(CPACK_WHEEL_PYTHON)
endif()

set(OPYN_WHEEL_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}/opynsim-python/lib/python3/site-packages")
set(OPYN_WHEEL_DIST_INFO_DIR "${CPACK_WHEEL_NAME}-${CPACK_WHEEL_VERSION}.dist-info")
set(OPYN_WHEEL_FILENAME "${CPACK_WHEEL_NAME}-${CPACK_WHEEL_VERSION}-${CPACK_WHEEL_TAG}.whl")
set(OPYN_WHEEL_OUTPUT_PATH "${CPACK_PACKAGE_DIRECTORY}/${OPYN_WHEEL_FILENAME}")

message(STATUS "Wheel source directory is ${OPYN_WHEEL_DIRECTORY}")

# Copy METADATA and WHEEL into the dist-info directory
file(
    COPY
        "${CPACK_WHEEL_METADATA_FILE}"
        "${CPACK_WHEEL_WHEEL_FILE}"
    DESTINATION ${OPYN_WHEEL_DIRECTORY}/${OPYN_WHEEL_DIST_INFO_DIR}
)

# Build the wheel
message(STATUS "wheel pack ${OPYN_WHEEL_DIRECTORY}")
execute_process(
    COMMAND "${CPACK_WHEEL_PYTHON}" -m wheel pack --dest-dir "${CPACK_PACKAGE_DIRECTORY}" "${OPYN_WHEEL_DIRECTORY}"
    RESULT_VARIABLE PY_RESULT
    OUTPUT_VARIABLE PY_OUT
    ERROR_VARIABLE PY_ERR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT PY_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Failed to build wheel:\n"
        "Exit code: ${PY_RESULT}\n"
        "Output:\n${PY_OUT}\n"
        "Error:\n${PY_ERR}"
    )
endif()
message(STATUS "wheel pack: OK")

# Run `twine check` on the wheel to validate METADATA/WHEEL a little
message(STATUS "twine check ${OPYN_WHEEL_OUTPUT_PATH}")
execute_process(
    COMMAND "${CPACK_WHEEL_PYTHON}" -m twine check "${OPYN_WHEEL_OUTPUT_PATH}"
    RESULT_VARIABLE PY_RESULT
    OUTPUT_VARIABLE PY_OUT
    ERROR_VARIABLE PY_ERR
)
if(NOT PY_RESULT EQUAL 0)
    message(FATAL_ERROR
            "Failed to twine check wheel:\n"
            "Exit code: ${PY_RESULT}\n"
            "Output:\n${PY_OUT}\n"
            "Error:\n${PY_ERR}"
    )
endif()
message(STATUS "twine check: OK")

# Run `pip check` on the wheel to validate requirements
message(STATUS "pip check ${OPYN_WHEEL_OUTPUT_PATH}")
execute_process(
        COMMAND "${CPACK_WHEEL_PYTHON}" -m pip check "${OPYN_WHEEL_OUTPUT_PATH}"
        RESULT_VARIABLE PY_RESULT
        OUTPUT_VARIABLE PY_OUT
        ERROR_VARIABLE PY_ERR
)
if(NOT PY_RESULT EQUAL 0)
    message(FATAL_ERROR
            "Failed to pip check wheel:\n"
            "Exit code: ${PY_RESULT}\n"
            "Output:\n${PY_OUT}\n"
            "Error:\n${PY_ERR}"
    )
endif()
message(STATUS "pip check: OK")

# Run custom `check_wheel_contents.py` to ensure that the wheel actually
# contains some of the expected entries (sanity check)
message(STATUS "check_wheel_contents.py ${OPYN_WHEEL_OUTPUT_PATH}")
execute_process(
        COMMAND "${CPACK_WHEEL_PYTHON}" "${CPACK_WHEEL_PACKAGING_SOURCE_DIR}/check_wheel_contents.py" "${OPYN_WHEEL_OUTPUT_PATH}" "opynsim/__init__.py" "opynsim/_opynsim_native.*" "opynsim/__main__.py"
        RESULT_VARIABLE PY_RESULT
        OUTPUT_VARIABLE PY_OUT
        ERROR_VARIABLE PY_ERR
)
if(NOT PY_RESULT EQUAL 0)
    message(FATAL_ERROR
            "Failed to pip check wheel:\n"
            "Exit code: ${PY_RESULT}\n"
            "Output:\n${PY_OUT}\n"
            "Error:\n${PY_ERR}"
    )
endif()
message(STATUS "pip check: OK")

message(STATUS "package: ${CPACK_PACKAGE_FILE_NAME} generated.")
