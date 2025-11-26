# Python binding packaging
#
# Package the python bindings into a standalone PyPi-compatible wheel
if(OPYN_BUILD_PYTHON_BINDINGS)

    # Useful for test.pypi.org (e.g. version.dev{1,2,3})
    set(OPYN_WHEEL_VERSION ${PROJECT_VERSION}.dev7)

    # Find Python
    find_package(Python REQUIRED COMPONENTS Interpreter)

    # Get `OPYN_PYTHON_INTERPRETER_TAG` from the native module
    get_target_property(_opyn_python_interpreter_tag _opynsim_native OPYN_PYTHON_INTERPRETER_TAG)
    if(${_opyn_python_interpreter_tag} STREQUAL "_opyn_python_interpreter_tag-NOTFOUND")
        message(FATAL_ERROR "OPYN_PYTHON_INTERPRETER_TAG property not set on _opynsim_native (${_opyn_python_interpreter_tag})")
    endif()

    # Get `OPYN_PYTHON_ABI_TAG` from the native module
    get_target_property(_opyn_python_abi_tag _opynsim_native OPYN_PYTHON_ABI_TAG)
    if(${_opyn_python_abi_tag} STREQUAL "_opyn_python_abi_tag-NOTFOUND")
        message(FATAL_ERROR "OPYN_PYTHON_ABI_TAG property not set on _opynsim_native (${_opyn_python_abi_tag})")
    endif()

    # Get `OPYN_PYTHON_PLATFORM_TAG` from the native module
    get_target_property(_opyn_python_platform_tag _opynsim_native OPYN_PYTHON_PLATFORM_TAG)
    if(${_opyn_python_platform_tag} STREQUAL "_opyn_python_platform_tag-NOTFOUND")
        message(FATAL_ERROR "OPYN_PYTHON_PLATFORM_TAG property not set on _opynsim_native (${_opyn_python_platform_tag})")
    endif()

    # If the native module was built with a stable ABI, then the `_opyn_wheel_tag` should be
    # updated to have a valid `abi3` field (PEP 384).
    if(_opyn_stable_abi)
        # Parse the Python tag of the form "impl-abi-platform"
        string(REPLACE "-" ";" _tag_parts "${_opyn_wheel_tag}")
        list(GET _tag_parts 2 _plat)
        string(REPLACE "." "" _opyn_interpreter_version "${OPYN_MIN_PYTHON_REQUIRED_VERSION}")
        set(_opyn_wheel_tag "cp${_opyn_interpreter_version}-abi3-${_plat}")

        unset(_tag_parts)
        unset(_plat)
        unset(_opyn_interpreter_version)
    endif()

    # Calculate OS-specific fields in package METADATA
    if(WIN32)
        set(OPYN_METADATA_PLATFORM "Windows")
        set(OPYN_METADATA_OS_CLASSIFIER "Operating System :: Microsoft :: Windows")
    elseif(UNIX AND NOT APPLE)
        set(OPYN_METADATA_PLATFORM "Linux")
        set(OPYN_METADATA_OS_CLASSIFIER "Operating System :: POSIX :: Linux")
    elseif(APPLE)
        set(OPYN_METADATA_PLATFORM "macOS")
        set(OPYN_METADATA_OS_CLASSIFIER "Operating System :: MacOS :: MacOS X")
    else()
        message(FATAL_ERROR "unsupported platform - review this code")
    endif()

    # Generate `dist-info` metadata files at configuration time, install them at install time
    configure_file(METADATA.in ${CMAKE_CURRENT_BINARY_DIR}/generated/METADATA @ONLY)
    configure_file(WHEEL.in    ${CMAKE_CURRENT_BINARY_DIR}/generated/WHEEL @ONLY)
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/generated/METADATA
            ${CMAKE_CURRENT_BINARY_DIR}/generated/WHEEL
        DESTINATION "${PROJECT_NAME}-${OPYN_WHEEL_VERSION}.dist-info"
        COMPONENT opynsim-python
    )
    set(CPACK_GENERATOR External)
    set(CPACK_EXTERNAL_PACKAGE "wheel")
    set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/PackageWheel.cmake")
    set(CPACK_WHEEL_PYTHON_EXECUTABLE "${Python_EXECUTABLE}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${OPYN_WHEEL_VERSION}-${OPYN_WHEEL_TAG}.whl")
    set(CPACK_COMPONENT_INSTALL ON)
    set(CPACK_EXTERNAL_ENABLE_STAGING YES)
    set(CPACK_EXTERNAL_PACKAGE_DEPENDS _opynsim_native)

    unset(_opyn_python_platform_tag)
    unset(_opyn_python_abi_tag)
    unset(_opyn_python_interpreter_tag)
endif()

include(CPack)
