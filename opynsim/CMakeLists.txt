include(CMakeDependentOption)

option(                OPYN_BUILD_OPYNSIM_DEBUGGER "Build the opynsim_debugger target" OFF)
cmake_dependent_option(OPYN_BUILD_PYTHON_TESTING   "Build python test suites" ON BUILD_TESTING OFF)
option(                OPYN_STABLE_ABI             "Perform a python stable ABI build" ON)

# Figure out required python components
set(OPYN_REQUIRED_PYTHON_COMPONENTS Interpreter)
if(OPYN_STABLE_ABI)
    set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.12)  # Dictated by nanobind
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.SABIModule)
else()
    set(OPYN_MIN_PYTHON_REQUIRED_VERSION 3.8)  # Dicatated by opynsim's python source code
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development.Module)
endif()
if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    list(APPEND OPYN_REQUIRED_PYTHON_COMPONENTS Development)  # the debugger uses embedded API
endif()

# Find Python
find_package(
    Python ${OPYN_MIN_PYTHON_REQUIRED_VERSION} REQUIRED
    COMPONENTS ${OPYN_REQUIRED_PYTHON_COMPONENTS}
)

# Build
#
# Build the native python extension module using `nanobind`
if(TRUE)
    # Include nanobind
    set(NB_USE_SUBMODULE_DEPS OFF)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nanobind ${CMAKE_CURRENT_BINARY_DIR}/opynsim/nanobind SYSTEM)

    # Use nanobind to declare the `_opynsim_native` Python extension module target
    set(OPYN_NANOBIND_ARGS NB_STATIC)
    if(OPYN_STABLE_ABI)
        list(APPEND OPYN_NANOBIND_ARGS STABLE_ABI)
    endif()
    nanobind_add_module(_opynsim_native ${OPYN_NANOBIND_ARGS}
        opynsim/src/_opynsim_native.cpp
    )

    # Set opynsim-specific properties on the native module:
    #
    #   OPYN_PYTHON_INTERPRETER_TAG   wheel-compatible interpreter tag (e.g. "cp311")
    #   OPYN_PYTHON_ABI_TAG           wheel-compatible ABI tag (e.g. "cp311", "abi3")
    #   OPYN_PYTHON_PLATFORM_TAG      wheel-compatible platform tag (e.g. "win_amd64", "manylinux_x86_64")
    if(TRUE)
        string(REPLACE "." "" _opyn_interpreter_tag "${OPYN_MIN_PYTHON_REQUIRED_VERSION}")
        set_target_properties(_opynsim_native PROPERTIES OPYN_PYTHON_INTERPRETER_TAG "cp${_opyn_interpreter_tag}")
        unset(_opyn_interpreter_tag)
    endif()
    if(OPYN_STABLE_ABI)
        set_target_properties(_opynsim_native PROPERTIES OPYN_PYTHON_ABI_TAG "abi3")
    else()
        # Remove dot between MAJOR.MINOR → "3.11" → "311"
        string(REGEX MATCH "^[0-9]+\\.[0-9]+" _py_mm "${Python_VERSION}")
        string(REPLACE "." "" _py_tag "${_py_mm}")

        set_target_properties(_opynsim_native PROPERTIES OPYN_PYTHON_ABI_TAG "cp${_py_tag}")

        unset(_py_tag)
        unset(_py_mm)
    endif()
    if(TRUE)
        # Detect Python wheel platform tag
        if(UNIX AND NOT APPLE)
            if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
                set(_plat_arch "x86_64")
            elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
                set(_plat_arch "aarch64")
            elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
                set(_plat_arch "armv7l")
            else()
                message(FATAL_ERROR "Unsupported Linux architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
            endif()

            set(_platform_tag "manylinux_${_plat_arch}")  # TODO: fix this

        elseif(APPLE)
            if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
                set(_plat_arch "arm64")
            else()
                set(_plat_arch "x86_64")
            endif()

            if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
                set(CMAKE_OSX_DEPLOYMENT_TARGET "${MACOSX_DEPLOYMENT_TARGET}")
            endif()

            string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _ UNUSED "${CMAKE_OSX_DEPLOYMENT_TARGET}")
            set(_mac_major "${CMAKE_MATCH_1}")
            set(_mac_minor "${CMAKE_MATCH_2}")

            set(_platform_tag "macosx_${_mac_major}_${_mac_minor}_${_plat_arch}")

        elseif(WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
                    set(_platform_tag "win_arm64")
                else()
                    set(_platform_tag "win_amd64")
                endif()
            else()
                set(_platform_tag "win32")
            endif()
        endif()
        set_target_properties(_opynsim_native PROPERTIES OPYN_PYTHON_PLATFORM_TAG ${_platform_tag})
        unset(_platform_tag)
    endif()

    # Link to the underlying `libopynsim` C++ code
    target_link_libraries(_opynsim_native PRIVATE opynsim)

    # Scrap all public symbols apart from `PyInit_*` / `_PyInit_*`
    #
    # This is to prevent symbol collisions when loading the library in a python
    # environment that might also contain (e.g.) OpenSim
    if (APPLE)
        target_link_options(_opynsim_native PRIVATE "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt")
    elseif(UNIX AND NOT APPLE)
        target_link_options(_opynsim_native PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
    endif()

    # Nice to have: Build `_opynsim_native` directly into the python package sourcetree
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
endif()

# Testing
if(OPYN_BUILD_PYTHON_TESTING)
    add_subdirectory(tests)
endif()

# Installation
#
# Creates a directory that has a package layout that can either be directly installed
# into the caller's (presumed, Linux) environment, or added to their PYTHONPATH
if(TRUE)
    set(OPYN_PYTHON_INSTALL_PREFIX "lib/python3/site-packages/opynsim")

    # Install package python sources (retain directory layout but skip src, __pycache__, etc.)
    file(GLOB_RECURSE OPYN_PYTHON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/opynsim/*.py")
    foreach(f ${OPYN_PYTHON_FILES})
        file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}/opynsim" "${f}")
        get_filename_component(dest_dir "${rel}" DIRECTORY)
        install(FILES "${f}" DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}/${dest_dir}" COMPONENT opynsim-python)
    endforeach()

    # Install `_opynsim_native` extension module into the package
    install(
        FILES $<TARGET_FILE:_opynsim_native>
        DESTINATION "${OPYN_PYTHON_INSTALL_PREFIX}/opynsim"
        COMPONENT opynsim-python
    )
endif()

if(OPYN_BUILD_OPYNSIM_DEBUGGER)
    add_subdirectory(debugger)
endif()
