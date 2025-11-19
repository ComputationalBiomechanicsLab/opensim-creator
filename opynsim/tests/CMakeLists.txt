option(OPYN_TEST_DEPENDENCIES "Add tests that check _opynsim_native's dependencies against a whitelist" ON)
option(OPYN_TEST_SYMBOLS      "Add tests that check _opynsim_native's symbols against a whitelist"      ON)

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
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "windows.txt")
    elseif(UNIX AND NOT APPLE)
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "linux.txt")
    elseif(APPLE)
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "macos.txt")
    endif()

    if(OPYN_DEPENDENCY_WHITELIST_FILENAME)
        add_test(
            NAME test_opynsim_native_dependencies_pass_whitelist
            COMMAND "${Python_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/check_dependencies.py"
                    "$<TARGET_FILE:_opynsim_native>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/dependency-whitelists/${OPYN_DEPENDENCY_WHITELIST_FILENAME}"
        )
    endif()
endif()

if(OPYN_TEST_SYMBOLS AND UNIX)
    if(APPLE)
        set(OPYN_PERMITTED_SYMBOLS "_PyInit__opynsim_native")
    else()
        set(OPYN_PERMITTED_SYMBOLS "PyInit__opynsim_native")
    endif()

    add_test(
        NAME test_opynsim_native_symbols_pass_whitelist
        COMMAND "${Python_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/check_symbols.py"
                "$<TARGET_FILE:_opynsim_native>"
                ${OPYN_PERMITTED_SYMBOLS}
    )
endif()
