# Perform runtime dynamic library dependency (dll/so/dylib) whitelist
# testing if `OSC_STRICT_ABI_CHECKS` is enabled.
if(OSC_STRICT_ABI_CHECKS)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    if(WIN32)
        set(OSC_DEPENDENCY_WHITELIST_FILENAME "windows.txt")
    elseif(UNIX AND NOT APPLE)
        set(OSC_DEPENDENCY_WHITELIST_FILENAME "linux.txt")
    elseif(APPLE)
        set(OSC_DEPENDENCY_WHITELIST_FILENAME "macos.txt")
    else()
        message(FATAL_ERROR "Unsupported platform detected: cannot perform OSC_STRICT_ABI_CHECKS: disable this option or fix this error!")
    endif()

    add_test(
        NAME test_core_dependencies_pass_whitelist
        COMMAND
            "${Python_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/opynsim/tests/check_dependencies.py"
            "--verbose"
            "$<TARGET_FILE:osc>"
            "${CMAKE_CURRENT_SOURCE_DIR}/dependency-whitelists/${OSC_DEPENDENCY_WHITELIST_FILENAME}"
    )
endif()

# Perform Apple SDK version checks, if applicable.
if(OSC_STRICT_ABI_CHECKS AND APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    add_test(
        NAME test_macos-targets-deployment-target
        COMMAND
        "${Python_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/opynsim/tests/check_macos-supports-min_os_version.py"
        "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        "$<TARGET_FILE:osc>"
    )
endif()
