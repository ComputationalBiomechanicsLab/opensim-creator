#include(CMakePrintHelpers)
#cmake_print_variables(CPACK_TEMPORARY_DIRECTORY)
#cmake_print_variables(CPACK_WHEEL_PYTHON_EXECUTABLE)
#cmake_print_variables(CPACK_TOPLEVEL_DIRECTORY)
#cmake_print_variables(CPACK_PACKAGE_DIRECTORY)
#cmake_print_variables(CPACK_PACKAGE_FILE_NAME)

message(STATUS "Wheel source directory is ${CPACK_TEMPORARY_DIRECTORY}")
execute_process(
    COMMAND "${CPACK_WHEEL_PYTHON_EXECUTABLE}" -m wheel pack --dest-dir "${CPACK_PACKAGE_DIRECTORY}" "${CPACK_TEMPORARY_DIRECTORY}"
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

message(STATUS "CPack: - package: ${CPACK_PACKAGE_FILE_NAME} generated.")
