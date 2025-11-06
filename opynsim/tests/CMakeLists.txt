find_package(Pytest)
if(NOT Pytest_FOUND)
    message(FATAL_ERROR
        "pytest is not installed in your current environment. You can solve this by either:\n"
        "  - Disabling python tests by setting the cmake cache variable `OPYN_BUILD_PYTHON_TESTING` to `OFF`\n"
        "  - Installing opynsim's test dependencies into the python environment with:\n"
        "        ${Python_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/../requirements/test_requirements.txt\n"
    )
endif()

file(GLOB OPYN_PYTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py")
pytest_discover_tests(
    _opynsim_native
    PYTHON_PATH_PREPEND
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        _opynsim_native
        ${OPYN_PYTEST_SOURCES}
    INCLUDE_FILE_PATH
)
unset(OPYN_PYTEST_SOURCES)
