# Perform runtime dynamic library dependency (dll/so/dylib) whitelist
# testing if `OSC_STRICT_ABI_CHECKS` is enabled.
if(OSC_STRICT_ABI_CHECKS)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    if(WIN32)
        set(_osc_dependency_whitelist_filename "windows.txt")
    elseif(UNIX AND NOT APPLE)
        set(_osc_dependency_whitelist_filename "linux.txt")
    elseif(APPLE)
        set(_osc_dependency_whitelist_filename "macos.txt")
    else()
        message(FATAL_ERROR "Unsupported platform detected: cannot perform OSC_STRICT_ABI_CHECKS: disable this option or fix this error!")
    endif()

    add_test(
        NAME test_osc_dependencies_pass_whitelist
        COMMAND
            "${Python_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/opynsim/tests/check_dependencies.py"
            "--verbose"
            "$<TARGET_FILE:osc>"
            "${CMAKE_CURRENT_SOURCE_DIR}/dependency-whitelists/${_osc_dependency_whitelist_filename}"
    )
endif()

# Perform Apple SDK version checks, if applicable.
if(OSC_STRICT_ABI_CHECKS AND APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    find_package(Python REQUIRED COMPONENTS Interpreter)

    add_test(
        NAME test_osc_macos-targets-deployment-target
        COMMAND
        "${Python_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/opynsim/tests/check_macos-supports-min_os_version.py"
        "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        "$<TARGET_FILE:osc>"
    )
endif()
