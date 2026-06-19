# Find `pytest` (python unit test library)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Pytest)
if(NOT Pytest_FOUND)
    message(FATAL_ERROR
        "pytest is not installed in your current environment. You can solve this by either:\n"
        "  - Disabling python tests by setting the cmake cache variable `OPYN_BUILD_PYTHON_TESTING` to `OFF`\n"
        "  - Installing pytest into your current environment (see ${CMAKE_CURRENT_SOURCE_DIR}/../requirements/test_requirements.txt)\n"
    )
endif()

# Discover `ptest` tests and add them to the CTest suite
file(GLOB OPYN_PYTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py")
pytest_discover_tests(
    _core
    PYTHON_PATH_PREPEND $<TARGET_FILE_DIR:_core>/..
    WORKING_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS             _core ${OPYN_PYTEST_SOURCES}
    INCLUDE_FILE_PATH
)
unset(OPYN_PYTEST_SOURCES)

# If requested, add additional dependency tests
if(OPYN_STRICT_ABI_CHECKS)
    if(WIN32)
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "windows.txt")
    elseif(UNIX AND NOT APPLE)
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "linux.txt")
    elseif(APPLE)
        set(OPYN_DEPENDENCY_WHITELIST_FILENAME "macos.txt")
    endif()

    if(OPYN_DEPENDENCY_WHITELIST_FILENAME)
        add_test(
            NAME test_core_dependencies_pass_whitelist
            COMMAND
                "${Python_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/check_dependencies.py"
                "--verbose"
                "$<TARGET_FILE:_core>"
                "${CMAKE_CURRENT_SOURCE_DIR}/dependency-whitelists/${OPYN_DEPENDENCY_WHITELIST_FILENAME}"
        )
    endif()
endif()

if(OPYN_STRICT_ABI_CHECKS AND UNIX)
    if(APPLE)
        set(OPYN_PERMITTED_SYMBOLS "_PyInit__core")
    else()
        set(OPYN_PERMITTED_SYMBOLS "PyInit__core")
    endif()

    add_test(
        NAME test_core_symbols_pass_whitelist
        COMMAND
            "${Python_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/check_symbols.py"
            "$<TARGET_FILE:_core>"
            ${OPYN_PERMITTED_SYMBOLS}
    )
endif()

# Perform Apple SDK version checks, if applicable.
if(OPYN_STRICT_ABI_CHECKS AND APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    add_test(
        NAME test_macos-targets-deployment-target
        COMMAND
        "${Python_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/check_macos-supports-min_os_version.py"
        "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        "$<TARGET_FILE:_core>"
    )
endif()
