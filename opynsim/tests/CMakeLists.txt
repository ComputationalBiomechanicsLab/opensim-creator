option(OPYN_TEST_DEPENDENCIES "Add tests that check _opynsim_native's dependencies" ON)

# Find `pytest` (python unit test library)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Pytest)
if(NOT Pytest_FOUND)
    message(FATAL_ERROR
        "pytest is not installed in your current environment. You can solve this by either:\n"
        "  - Disabling python tests by setting the cmake cache variable `OPYN_BUILD_PYTHON_TESTING` to `OFF`\n"
        "  - Installing opynsim's test dependencies into the python environment with:\n"
        "        ${Python_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/../requirements/test_requirements.txt\n"
    )
endif()

# Discover `ptest` tests and add them to the CTest suite
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

# If requested, add additional dependency tests
if(OPYN_TEST_DEPENDENCIES)
    if(WIN32)
        add_test(
            NAME test_windows_opynsim_native_dependencies_pass_whitelist
            COMMAND "${Python_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/check_dependencies.py"
                    "$<TARGET_FILE:_opynsim_native>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/windows_whitelist.txt"
        )
    elseif(UNIX AND NOT APPLE)
        add_test(
            NAME test_linux_opynsim_native_dependencies_pass_whitelist
            COMMAND "${Python_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/check_dependencies.py"
                    "$<TARGET_FILE:_opynsim_native>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/linux_whitelist.txt"
        )
    endif()
endif()
