# Python binding packaging
#
# Package the python bindings into a standalone PyPi-compatible wheel
if(OPYN_BUILD_PYTHON_BINDINGS)

    # Useful for test.pypi.org (e.g. version.dev{1,2,3})
    set(OPYN_WHEEL_VERSION ${PROJECT_VERSION})

    # Find Python
    find_package(Python REQUIRED COMPONENTS Interpreter)

    # Get `OPYN_PYTHON_INTERPRETER_TAG` from the native module
    get_target_property(_opyn_python_interpreter_tag _core OPYN_PYTHON_INTERPRETER_TAG)
    if(_opyn_python_interpreter_tag STREQUAL "_opyn_python_interpreter_tag-NOTFOUND")
        message(FATAL_ERROR "OPYN_PYTHON_INTERPRETER_TAG property not set on _core (${_opyn_python_interpreter_tag})")
    endif()

    # Get `OPYN_PYTHON_ABI_TAG` from the native module
    get_target_property(_opyn_python_abi_tag _core OPYN_PYTHON_ABI_TAG)
    if(_opyn_python_abi_tag STREQUAL "_opyn_python_abi_tag-NOTFOUND")
        message(FATAL_ERROR "OPYN_PYTHON_ABI_TAG property not set on _core (${_opyn_python_abi_tag})")
    endif()

    # Get `_opyn_python_platform_tag` from Python itself or calculate it
    # for macOS (because macOS is more common for cross-compilation, or
    # different target OS versions, etc.).
    if(APPLE)
        if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
            set(_mac_arch "arm64")
        elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
            set(_mac_arch "x86_64")
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")  # fallback to host if not specified
            set(_mac_arch "arm64")
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
            set(_mac_arch "x86_64")
        else()
            message(FATAL_ERROR "Cannot deduce macOS architecture (CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}, CMAKE_HOST_SYSTEM_PROCESSOR=${CMAKE_HOST_SYSTEM_PROCESSOR})")
        endif()

        if(CMAKE_OSX_DEPLOYMENT_TARGET)
            set(_mac_target "${CMAKE_OSX_DEPLOYMENT_TARGET}")
        elseif(CMAKE_SYSTEM_VERSION)
            set(_mac_target "${CMAKE_SYSTEM_VERSION}")
        else()
            message(FATAL_ERROR "Cannot deduce target macOS system version (CMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}, CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION})")
        endif()
        string(REPLACE "." "_" _mac_target_underscored "${_mac_target}")

        set(_opyn_python_platform_tag "macosx_${_mac_target_underscored}_${_mac_arch}")
    else()
        # Windows/Linux/other - get it from Python
        execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "from packaging.tags import sys_tags; print(next(sys_tags()).platform)"
            OUTPUT_VARIABLE _opyn_python_platform_tag
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_VARIABLE PY_ERR
            RESULT_VARIABLE PY_RESULT
        )
        if(NOT PY_RESULT EQUAL 0)
            message(FATAL_ERROR
                    "Failed to query Python platform with ${Python_EXECUTABLE}:\n"
                    "Exit code: ${PY_RESULT}\n"
                    "Error output:\n${PY_ERR}"
            )
        endif()
        unset(PY_ERR)
        unset(PY_RESULT)
    endif()

    # Calculate wheel tag/filename that's suitable for the native module
    set(OPYN_WHEEL_TAG "${_opyn_python_interpreter_tag}-${_opyn_python_abi_tag}-${_opyn_python_platform_tag}")
    set(OPYN_WHEEL_FILENAME "${PROJECT_NAME}-${OPYN_WHEEL_VERSION}-${OPYN_WHEEL_TAG}.whl")

    # Calculate OS-specific fields in the package METADATA
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

    # Generate `METADATA` and `WHEEL` files so that `PackageWheel.cmake` can copy them
    # into `.dist-info`
    configure_file(METADATA.in ${CMAKE_CURRENT_BINARY_DIR}/generated/METADATA @ONLY)
    configure_file(WHEEL.in    ${CMAKE_CURRENT_BINARY_DIR}/generated/WHEEL @ONLY)

    # Setup CPack to generate a wheel
    set(CPACK_GENERATOR External)
    set(CPACK_EXTERNAL_PACKAGE "wheel")
    set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/PackageWheel.cmake")
    set(CPACK_EXTERNAL_ENABLE_STAGING YES)
    set(CPACK_EXTERNAL_PACKAGE_DEPENDS _core)
    set(CPACK_PACKAGE_FILE_NAME "${OPYN_WHEEL_FILENAME}")
    set(CPACK_COMPONENT_INSTALL ON)

    set(CPACK_WHEEL_PACKAGING_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(CPACK_WHEEL_METADATA_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/METADATA")
    set(CPACK_WHEEL_WHEEL_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/WHEEL")
    set(CPACK_WHEEL_NAME "${PROJECT_NAME}")
    set(CPACK_WHEEL_VERSION "${OPYN_WHEEL_VERSION}")
    set(CPACK_WHEEL_TAG "${OPYN_WHEEL_TAG}")
    set(CPACK_WHEEL_PYTHON "${Python_EXECUTABLE}")
    set(CPACK_WHEEL_DEBUG OFF)

    unset(OPYN_METADATA_OS_CLASSIFIER)
    unset(OPYN_METADATA_PLATFORM)
    unset(OPYN_WHEEL_FILENAME)
    unset(OPYN_WHEEL_TAG)
    unset(_opyn_python_platform_tag)
    unset(_opyn_python_abi_tag)
    unset(_opyn_python_interpreter_tag)
endif()

include(CPack)
