include(CMakeDependentOption)

option(                OPYN_BUILD_OPYNSIM_DEBUGGER "Build the opynsim_debugger target" ON)
cmake_dependent_option(OPYN_BUILD_PYTHON_TESTING   "Build python test suites" ON BUILD_TESTING OFF)

# This indirection can be useful when developing the package for PyPi etc. without having
# to bump the cmake version and rebuild the entire project
set(OPYN_WHEEL_VERSION ${PROJECT_VERSION}.dev6)

# Find Python
find_package(
    Python 3.8 REQUIRED
    COMPONENTS Interpreter Development Development.Module Development.SABIModule
)

# Include `nanobind` (python binding library)
if(TRUE)
    set(NB_USE_SUBMODULE_DEPS OFF)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opynsim/nanobind SYSTEM)
endif()

# Use nanobind to build the `_opynsim_native` Python extension module
nanobind_add_module(_opynsim_native NB_STATIC # STABLE_ABI TODO

    opynsim/src/_opynsim_native.cpp
)
target_link_libraries(_opynsim_native PRIVATE opynsim)

# Build `_opynsim_native` directly into the python package sourcetree
#
# This is purely a practical measure that enables developers to run a Python
# REPL, tests, etc. from a terminal without having to re-run a built/copy/install
# script after every python edit.
#
# Technical note:
#     > Multi-configuration generators (Visual Studio, Xcode, Ninja Multi-Config) append
#     > a per-configuration subdirectory to the specified directory ***unless a generator
#     > expression is used***.
#         - https://cmake.org/cmake/help/latest/prop_tgt/LIBRARY_OUTPUT_DIRECTORY.html
set_target_properties(_opynsim_native PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
    RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}>/opynsim/
)

# Installation
#
# Creates a directory that has a package + dist-info layout that can either be copied
# into the caller's site-packages directory or zipped up into a wheel for packaging.
if(TRUE)
    # Install package python sources (retain directory layout but skip src, __pycache__, etc.)
    file(GLOB_RECURSE OPYN_PYTHON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/opynsim/*.py")
    foreach(f ${OPYN_PYTHON_FILES})
        file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/opynsim" "${f}")
        get_filename_component(dest_dir "${rel}" DIRECTORY)
        install(FILES "${f}" DESTINATION "opynsim/${dest_dir}")
    endforeach()

    # Install `_opynsim_native` extension module into the package
    install(
        TARGETS _opynsim_native
        LIBRARY DESTINATION "opynsim"
        RUNTIME DESTINATION "opynsim"
    )

    # Configure-time: Compute `OPYN_WHEEL_TAG`
    if(TRUE)
        execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "from packaging import tags; print(next(tags.sys_tags()))"
            OUTPUT_VARIABLE OPYN_WHEEL_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_VARIABLE PY_ERR
            RESULT_VARIABLE PY_RESULT
        )
        if(NOT PY_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Failed to query Python tags with ${Python_EXECUTABLE}:\n"
                "Exit code: ${PY_RESULT}\n"
                "Error output:\n${PY_ERR}"
            )
        endif()
    endif()

    # Calculate OS-specific fields in package METADATA
    if(WIN32)
        set(OPYN_METADATA_PLATFORM "Windows")
        set(OPYN_METADATA_OS_CLASSIFIER "Operating System :: Microsoft :: Windows")
    elseif(UNIX AND NOT APPLE)
        set(OPYN_METADATA_PLATFORM "Linux")
        set(OPYN_METADATA_OS_CLASSIFIER "POSIX :: Linux")
    elseif(APPLE)
        set(OPYN_METADATA_PLATFORM "macOS")
        set(OPYN_METADATA_OS_CLASSIFIER "MacOS :: MacOS X")
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
    )
endif()

# Packaging
#
# Zips up the installation directory structure into a wheel.
if(TRUE)
    set(CPACK_GENERATOR External)
    set(CPACK_EXTERNAL_PACKAGE "wheel")
    set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/PackageWheel.cmake")
    set(CPACK_WHEEL_PYTHON_EXECUTABLE "${Python_EXECUTABLE}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${OPYN_WHEEL_VERSION}-${OPYN_WHEEL_TAG}.whl")
    set(CPACK_EXTERNAL_ENABLE_STAGING YES)
    set(CPACK_EXTERNAL_PACKAGE_DEPENDS _opynsim_native)
    include(CPack)

    set(OPYN_BUILT_WHEEL_PATH ${CMAKE_BINARY_DIR}/"${PROJECT_NAME}-${OPYN_WHEEL_VERSION}-${OPYN_WHEEL_TAG}.whl")
endif()

# Deployment
#
# Deploys the wheel to PyPi (if desired)
if(TRUE)
    # This assumes the caller has a valid environment (TWINE_USERNAME and TWINE_PASSWORD set, or a config file)
    add_custom_target(deploy_to_testpypi
        COMMAND ${Python_EXECUTABLE} -m twine upload --verbose --repository-url https://test.pypi.org/legacy/ ${OPYN_BUILT_WHEEL_PATH}
        DEPENDS package
        COMMENT "Upload built wheel to test.pypi.org (TWINE_USERNAME+TWINE_PASSWORD, or ~/.pypirc must be configured)"
    )

    add_custom_target(deploy_to_pypi
        COMMAND ${Python_EXECUTABLE} -m twine upload --verbose --repository-url https://pypi.org/legacy/ ${OPYN_BUILT_WHEEL_PATH}
        DEPENDS package
        COMMENT "Upload built wheel to pypi.org (TWINE_USERNAME+TWINE_PASSWORD, or ~/.pypirc must be configured)"
    )
endif()

if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    add_subdirectory(debugger)
endif()

if(OPYN_BUILD_PYTHON_TESTING)
    add_subdirectory(tests)
endif()
